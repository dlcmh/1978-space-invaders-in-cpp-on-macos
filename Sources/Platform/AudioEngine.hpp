#pragma once

#include <AudioToolbox/AudioToolbox.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace invaders::platform {

enum class Sound : std::uint8_t {
    PlayerFire,
    AlienExplode,
    PlayerExplode,
    UfoHum,
    UfoExplode,
    March,
    WaveClear,
    GameOver,
    BunkerHit,
    Start,
};

/// A small procedural sound chip built on Core Audio's default output unit.
/// No audio files, no third-party engine, and no network access. Events enter
/// through a single-producer/single-consumer queue; the real-time callback
/// synthesizes square, saw, sine, and noise voices with no allocations.
class AudioEngine final {
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    auto operator=(const AudioEngine&) -> AudioEngine& = delete;
    AudioEngine(AudioEngine&&) = delete;
    auto operator=(AudioEngine&&) -> AudioEngine& = delete;

    void play(Sound sound) noexcept;
    void setMuted(bool muted) noexcept;
    [[nodiscard]] auto muted() const noexcept -> bool {
        return muted_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] auto available() const noexcept -> bool { return available_; }

private:
    struct Voice {
        Sound sound = Sound::March;
        bool active = false;
        float phase = 0.0F;
        float age = 0.0F;
        float duration = 0.1F;
    };

    static auto render(void* context, AudioUnitRenderActionFlags* actionFlags,
                       const AudioTimeStamp* timeStamp, UInt32 busNumber, UInt32 frameCount,
                       AudioBufferList* bufferList) -> OSStatus;

    void renderFrames(float* output, UInt32 frameCount) noexcept;
    void trigger(Sound sound) noexcept;
    void drainRequests() noexcept;

    static constexpr std::size_t kQueueCapacity = 64;
    static constexpr std::size_t kVoiceCount = 12;
    static constexpr float kSampleRate = 48'000.0F;

    void* audioUnit_ = nullptr;
    bool available_ = false;
    std::atomic<bool> muted_{false};
    std::array<Sound, kQueueCapacity> queue_{};
    std::atomic<std::uint32_t> writeIndex_{0};
    std::atomic<std::uint32_t> readIndex_{0};
    std::array<Voice, kVoiceCount> voices_{};
    std::uint32_t noiseState_ = 0x1978'5A17U;
};

} // namespace invaders::platform
