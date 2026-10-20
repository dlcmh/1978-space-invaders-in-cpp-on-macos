#include "AudioEngine.hpp"

#include <AudioToolbox/AudioToolbox.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <numbers>

namespace invaders::platform {
namespace {

constexpr float kMasterGain = 0.42F;

auto noiseFrom(std::uint32_t& state) noexcept -> float {
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return static_cast<float>(state & 0xFFFFU) / 32'767.5F - 1.0F;
}

auto square(float phase) noexcept -> float {
    return phase < 0.5F ? 1.0F : -1.0F;
}

auto saw(float phase) noexcept -> float {
    return phase * 2.0F - 1.0F;
}

auto sine(float phase) noexcept -> float {
    return std::sin(phase * std::numbers::pi_v<float> * 2.0F);
}

auto soundDuration(Sound sound) noexcept -> float {
    switch (sound) {
    case Sound::PlayerFire:
        return 0.18F;
    case Sound::AlienExplode:
        return 0.24F;
    case Sound::PlayerExplode:
        return 0.9F;
    case Sound::UfoHum:
        return 1.7F;
    case Sound::UfoExplode:
        return 0.55F;
    case Sound::March:
        return 0.085F;
    case Sound::WaveClear:
        return 1.0F;
    case Sound::GameOver:
        return 1.25F;
    case Sound::BunkerHit:
        return 0.11F;
    case Sound::Start:
        return 0.45F;
    }
    return 0.1F;
}

} // namespace

AudioEngine::AudioEngine() {
    AudioComponentDescription description{};
    description.componentType = kAudioUnitType_Output;
    description.componentSubType = kAudioUnitSubType_DefaultOutput;
    description.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent component = AudioComponentFindNext(nullptr, &description);
    if (component == nullptr) {
        return;
    }
    AudioComponentInstance unit = nullptr;
    if (AudioComponentInstanceNew(component, &unit) != noErr || unit == nullptr) {
        return;
    }
    audioUnit_ = unit;

    AudioStreamBasicDescription format{};
    format.mSampleRate = kSampleRate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags =
        kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
    format.mBytesPerPacket = sizeof(float);
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = sizeof(float);
    format.mChannelsPerFrame = 1;
    format.mBitsPerChannel = 32;

    if (AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0,
                             &format, sizeof(format)) != noErr) {
        AudioComponentInstanceDispose(unit);
        audioUnit_ = nullptr;
        return;
    }

    AURenderCallbackStruct callback{};
    callback.inputProc = &AudioEngine::render;
    callback.inputProcRefCon = this;
    if (AudioUnitSetProperty(unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0,
                             &callback, sizeof(callback)) != noErr) {
        AudioComponentInstanceDispose(unit);
        audioUnit_ = nullptr;
        return;
    }

    if (AudioUnitInitialize(unit) != noErr) {
        AudioComponentInstanceDispose(unit);
        audioUnit_ = nullptr;
        return;
    }
    if (AudioOutputUnitStart(unit) != noErr) {
        AudioUnitUninitialize(unit);
        AudioComponentInstanceDispose(unit);
        audioUnit_ = nullptr;
        return;
    }
    available_ = true;
}

AudioEngine::~AudioEngine() {
    if (audioUnit_ != nullptr) {
        auto unit = static_cast<AudioComponentInstance>(audioUnit_);
        AudioOutputUnitStop(unit);
        AudioUnitUninitialize(unit);
        AudioComponentInstanceDispose(unit);
        audioUnit_ = nullptr;
    }
}

void AudioEngine::play(Sound sound) noexcept {
    if (!available_ || muted_.load(std::memory_order_relaxed)) {
        return;
    }
    const std::uint32_t write = writeIndex_.load(std::memory_order_relaxed);
    const std::uint32_t next = (write + 1U) % static_cast<std::uint32_t>(kQueueCapacity);
    if (next == readIndex_.load(std::memory_order_acquire)) {
        return;
    }
    queue_[static_cast<std::size_t>(write)] = sound;
    writeIndex_.store(next, std::memory_order_release);
}

void AudioEngine::setMuted(bool muted) noexcept {
    muted_.store(muted, std::memory_order_relaxed);
}

void AudioEngine::drainRequests() noexcept {
    if (muted_.load(std::memory_order_relaxed)) {
        readIndex_.store(writeIndex_.load(std::memory_order_acquire), std::memory_order_release);
        return;
    }
    std::uint32_t read = readIndex_.load(std::memory_order_relaxed);
    const std::uint32_t write = writeIndex_.load(std::memory_order_acquire);
    while (read != write) {
        trigger(queue_[static_cast<std::size_t>(read)]);
        read = (read + 1U) % static_cast<std::uint32_t>(kQueueCapacity);
    }
    readIndex_.store(read, std::memory_order_release);
}

void AudioEngine::trigger(Sound sound) noexcept {
    Voice* chosen = nullptr;
    for (Voice& voice : voices_) {
        if (!voice.active) {
            chosen = &voice;
            break;
        }
    }
    if (chosen == nullptr) {
        chosen = &voices_.front();
    }
    chosen->sound = sound;
    chosen->active = true;
    chosen->phase = 0.0F;
    chosen->age = 0.0F;
    chosen->duration = soundDuration(sound);
}

void AudioEngine::renderFrames(float* output, UInt32 frameCount) noexcept {
    std::fill(output, output + static_cast<std::size_t>(frameCount), 0.0F);
    if (muted_.load(std::memory_order_relaxed)) {
        return;
    }
    drainRequests();

    constexpr float inverseSampleRate = 1.0F / kSampleRate;
    for (UInt32 frame = 0; frame < frameCount; ++frame) {
        float mixed = 0.0F;
        for (Voice& voice : voices_) {
            if (!voice.active) {
                continue;
            }
            if (voice.age >= voice.duration) {
                voice.active = false;
                continue;
            }
            const float progress = voice.age / voice.duration;
            const float envelope = (1.0F - progress) * (1.0F - progress);
            float sample = 0.0F;

            switch (voice.sound) {
            case Sound::PlayerFire: {
                const float frequency = 1050.0F - progress * 700.0F;
                sample = square(voice.phase) * 0.28F * envelope;
                voice.phase += frequency * inverseSampleRate;
                break;
            }
            case Sound::AlienExplode: {
                const float frequency = 180.0F - progress * 110.0F;
                const float grit = noiseFrom(noiseState_);
                sample = (square(voice.phase) * 0.18F + grit * 0.22F) * envelope;
                voice.phase += frequency * inverseSampleRate;
                break;
            }
            case Sound::PlayerExplode: {
                const float frequency = 220.0F - progress * 175.0F;
                const float grit = noiseFrom(noiseState_);
                sample = (saw(voice.phase) * 0.25F + grit * 0.18F) * envelope;
                voice.phase += frequency * inverseSampleRate;
                break;
            }
            case Sound::UfoHum: {
                const float wobble = std::sin(voice.age * 25.0F) * 65.0F;
                const float frequency = 260.0F + wobble;
                const float pulse = 0.55F + 0.45F * square(voice.phase * 4.0F);
                sample = sine(voice.phase) * 0.14F * pulse * envelope;
                voice.phase += frequency * inverseSampleRate;
                break;
            }
            case Sound::UfoExplode: {
                const float frequency = 900.0F - progress * 620.0F;
                const float grit = noiseFrom(noiseState_);
                sample = (sine(voice.phase) * 0.2F + grit * 0.16F) * envelope;
                voice.phase += frequency * inverseSampleRate;
                break;
            }
            case Sound::March: {
                sample = square(voice.phase) * 0.17F * envelope;
                voice.phase += 68.0F * inverseSampleRate;
                break;
            }
            case Sound::WaveClear: {
                constexpr std::array<float, 4> notes = {523.25F, 659.25F, 783.99F, 1046.5F};
                const auto index = static_cast<std::size_t>(std::min(3.0F, progress * 4.0F));
                sample = square(voice.phase) * 0.18F * envelope;
                voice.phase += notes[index] * inverseSampleRate;
                break;
            }
            case Sound::GameOver: {
                const float frequency = 180.0F - progress * 125.0F;
                sample = saw(voice.phase) * 0.22F * envelope;
                voice.phase += frequency * inverseSampleRate;
                break;
            }
            case Sound::BunkerHit: {
                sample = noiseFrom(noiseState_) * 0.15F * envelope;
                break;
            }
            case Sound::Start: {
                constexpr std::array<float, 3> notes = {392.0F, 523.25F, 659.25F};
                const auto index = static_cast<std::size_t>(std::min(2.0F, progress * 3.0F));
                sample = square(voice.phase) * 0.17F * envelope;
                voice.phase += notes[index] * inverseSampleRate;
                break;
            }
            }

            if (voice.phase >= 1.0F) {
                voice.phase -= std::floor(voice.phase);
            }
            voice.age += inverseSampleRate;
            mixed += sample;
        }
        output[frame] = std::clamp(mixed * kMasterGain, -1.0F, 1.0F);
    }
}

auto AudioEngine::render(void* context, AudioUnitRenderActionFlags* actionFlags,
                         const AudioTimeStamp* timeStamp, UInt32 busNumber, UInt32 frameCount,
                         AudioBufferList* bufferList) -> OSStatus {
    (void)actionFlags;
    (void)timeStamp;
    (void)busNumber;
    auto* engine = static_cast<AudioEngine*>(context);
    auto* buffers = bufferList;
    if (engine == nullptr || buffers == nullptr || buffers->mNumberBuffers == 0) {
        return noErr;
    }
    auto* samples = static_cast<float*>(buffers->mBuffers[0].mData);
    if (samples == nullptr) {
        return noErr;
    }
    engine->renderFrames(samples, frameCount);
    return noErr;
}

} // namespace invaders::platform
