# How does procedural audio work without files?

**Short answer:** the app runs a single-channel Core Audio output unit at
48 kHz. Game events enqueue sound IDs in a lock-free ring; the real-time callback
synthesizes square, saw, sine, and noise voices with a short envelope. No audio
files, sample packs, or third-party engine are involved.

## Event to sound

[GameView.mm](../../Sources/Platform/GameView.mm) — `playEventSound` maps simulation
events to `Sound` values: player fire, alien explosion, march, player
explosion, mystery-ship appearance and destruction, wave clear, game over, and
start. This is deliberately one-way. The simulation never asks whether audio is
available.

## The real-time boundary

[AudioEngine.cpp](../../Sources/Platform/AudioEngine.cpp) — `AudioEngine::play` pushes
a sound onto a fixed-size single-producer/single-consumer queue. The render
callback calls `AudioEngine::renderFrames`, which:

- fills the output buffer with silence;
- drains any new sound requests;
- updates a fixed array of 12 voices; and
- mixes their samples with a master gain, clamping to [-1, 1].

The callback does no allocation, locking, or Objective-C message sends. Voices
are recycled; if all 12 are busy, the oldest slot is reused. The engine also
clears the queue when muted so queued sounds do not play on unmute.

## Synthesis

Each voice is a phase accumulator plus an age and duration. The sound type
selects a recipe:

- **player fire** — descending square chirp;
- **alien explosion** — low square plus noise;
- **player explosion** — descending saw plus noise;
- **mystery ship** — warbling sine hum;
- **UFO destruction** — bright descending sine plus noise;
- **march** — very short low square thump;
- **wave clear** — four-note square arpeggio;
- **game over** — descending saw; and
- **bunker hit** — a filtered noise tick.

The envelope is `(1 - progress)^2`, which removes clicks at the end of each
voice.

## Failure handling

If Core Audio cannot create or start the default output unit, the constructor
leaves the engine unavailable and every `play` becomes a no-op. The game keeps
running silently. Mute is an atomic flag read by the callback and is persisted
as `SpaceInvadersCPPMuted`; audio is entirely local and no microphone or input
permission is requested.
