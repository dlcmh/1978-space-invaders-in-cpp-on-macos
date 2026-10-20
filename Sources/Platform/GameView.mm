#import "GameView.h"

#import "MetalRenderer.h"
#import <AppKit/AppKit.h>

#include "AudioEngine.hpp"

#include "invaders/Game.hpp"

#include <QuartzCore/QuartzCore.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace {

constexpr double kFixedStep = 1.0 / 60.0;
NSString* const kHighScoreKey = @"SpaceInvadersCPPHighScore";
NSString* const kMutedKey = @"SpaceInvadersCPPMuted";
NSString* const kErrorDomain = @"dev.dlcmh.space-invaders.gameview";

NSError* viewError(NSString* description) {
    NSDictionary<NSString*, id>* info = @{NSLocalizedDescriptionKey : description};
    return [NSError errorWithDomain:kErrorDomain code:1 userInfo:info];
}

void playEventSound(invaders::platform::AudioEngine& audio, const invaders::Event& event) {
    using invaders::EventType;
    using invaders::platform::Sound;
    switch (event.type) {
    case EventType::GameStarted:
        audio.play(Sound::Start);
        break;
    case EventType::PlayerFired:
        audio.play(Sound::PlayerFire);
        break;
    case EventType::AlienMarch:
        audio.play(Sound::March);
        break;
    case EventType::AlienHit:
        audio.play(Sound::AlienExplode);
        break;
    case EventType::BunkerHit:
        audio.play(Sound::BunkerHit);
        break;
    case EventType::PlayerHit:
        audio.play(Sound::PlayerExplode);
        break;
    case EventType::UfoAppeared:
        audio.play(Sound::UfoHum);
        break;
    case EventType::UfoHit:
        audio.play(Sound::UfoExplode);
        break;
    case EventType::WaveCleared:
        audio.play(Sound::WaveClear);
        break;
    case EventType::GameOver:
        audio.play(Sound::GameOver);
        break;
    }
}

} // namespace

@implementation SIGameView {
    std::unique_ptr<invaders::Game> _game;
    std::unique_ptr<invaders::platform::AudioEngine> _audio;
    SIRenderer* _renderer;

    double _lastFrameTime;
    double _accumulator;
    double _lastHighScoreSave;
    int _lastSavedHighScore;
    BOOL _autoPauseOnResign;
    BOOL _muted;
}

- (nullable instancetype)initWithFrame:(NSRect)frame error:(NSError* _Nullable* _Nullable)error {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) {
        if (error != nullptr) {
            *error = viewError(@"This Mac does not expose a Metal device.");
        }
        return nil;
    }

    self = [super initWithFrame:frame device:device];
    if (self == nil) {
        return nil;
    }

    self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.sampleCount = [device supportsTextureSampleCount:4] ? 4 : 1;
    self.clearColor = MTLClearColorMake(0.002, 0.006, 0.012, 1.0);
    self.framebufferOnly = YES;
    self.enableSetNeedsDisplay = NO;
    self.paused = NO;
    self.preferredFramesPerSecond = 60;
    self.autoResizeDrawable = YES;
    self.layer.backgroundColor = NSColor.blackColor.CGColor;
    self.wantsLayer = YES;

    NSUserDefaults* defaults = NSUserDefaults.standardUserDefaults;
    const int savedHighScore = static_cast<int>([defaults integerForKey:kHighScoreKey]);
    _muted = [defaults boolForKey:kMutedKey];
    _autoPauseOnResign =
        ![NSProcessInfo.processInfo.environment[@"SPACEINVADERS_AUTOPAUSE"] isEqualToString:@"0"];

    _game = std::make_unique<invaders::Game>(invaders::GameOptions{
        .seed = 0x1978'5ACE'CAFE'BEEFULL,
        .startingLives = 3,
        .highScore = savedHighScore,
    });
    _lastSavedHighScore = savedHighScore;
    _lastHighScoreSave = CACurrentMediaTime();

    _audio = std::make_unique<invaders::platform::AudioEngine>();
    _audio->setMuted(_muted == YES);

    NSError* rendererError = nil;
    _renderer = [[SIRenderer alloc] initWithView:self error:&rendererError];
    if (_renderer == nil) {
        if (error != nullptr) {
            *error = rendererError != nil ? rendererError
                                          : viewError(@"Metal renderer initialization failed.");
        }
        return nil;
    }
    [_renderer setMuted:_muted];

    self.delegate = self;

    if ([NSProcessInfo.processInfo.environment[@"SPACEINVADERS_AUTOSTART"] isEqualToString:@"1"]) {
        _game->startNewGame();
    }

    NSNotificationCenter* center = NSNotificationCenter.defaultCenter;
    [center addObserver:self
               selector:@selector(applicationDidResignActive:)
                   name:NSApplicationDidResignActiveNotification
                 object:nil];

    return self;
}

- (void)dealloc {
    [NSNotificationCenter.defaultCenter removeObserver:self];
    [self saveHighScore];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    self.window.backgroundColor = NSColor.blackColor;
    self.window.acceptsMouseMovedEvents = NO;
    [self.window makeFirstResponder:self];
}

- (void)applicationDidResignActive:(NSNotification*)notification {
    (void)notification;
    if (_autoPauseOnResign && _game != nullptr) {
        _game->pause();
        _game->setMoveLeft(false);
        _game->setMoveRight(false);
        _game->setFiring(false);
    }
}

- (void)drawInMTKView:(MTKView*)view {
    if (_game == nullptr || _renderer == nil) {
        return;
    }
    const double now = CACurrentMediaTime();
    if (_lastFrameTime <= 0.0) {
        _lastFrameTime = now;
    }
    const double frameDelta = std::min(std::max(now - _lastFrameTime, 0.0), 0.1);
    _lastFrameTime = now;
    _accumulator += frameDelta;

    int steps = 0;
    while (_accumulator >= kFixedStep && steps < 8) {
        _game->update(static_cast<float>(kFixedStep));
        for (const invaders::Event& event : _game->drainEvents()) {
            playEventSound(*_audio, event);
            [_renderer handleEvent:event];
        }
        _accumulator -= kFixedStep;
        ++steps;
    }
    if (steps == 8) {
        _accumulator = 0.0;
    }

    [_renderer renderGame:*_game inView:view deltaSeconds:static_cast<float>(frameDelta)];

    const int highScore = _game->highScore();
    if (highScore > _lastSavedHighScore) {
        if (_game->phase() == invaders::Phase::GameOver || now - _lastHighScoreSave > 1.5) {
            [self saveHighScore];
        }
    }
}

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    (void)view;
    (void)size;
}

- (void)keyDown:(NSEvent*)event {
    if ((event.modifierFlags & NSEventModifierFlagCommand) != 0) {
        [super keyDown:event];
        return;
    }

    const unsigned short code = event.keyCode;
    switch (code) {
    case 123: // left arrow
    case 0:   // A
        _game->setMoveLeft(true);
        break;
    case 124: // right arrow
    case 2:   // D
        _game->setMoveRight(true);
        break;
    case 49: { // space
        const invaders::Phase phase = _game->phase();
        if (phase == invaders::Phase::Attract || phase == invaders::Phase::GameOver) {
            [self newGame];
        } else {
            _game->setFiring(true);
        }
        break;
    }
    case 36:
    case 76: { // return / keypad enter
        const invaders::Phase phase = _game->phase();
        if (phase == invaders::Phase::Attract || phase == invaders::Phase::GameOver) {
            [self newGame];
        }
        break;
    }
    case 35: // P
        _game->togglePause();
        break;
    case 53: // Escape
        _game->pause();
        break;
    case 46: // M
        [self toggleMute];
        break;
    default:
        break;
    }
}

- (void)keyUp:(NSEvent*)event {
    switch (event.keyCode) {
    case 123:
    case 0:
        _game->setMoveLeft(false);
        break;
    case 124:
    case 2:
        _game->setMoveRight(false);
        break;
    case 49:
        _game->setFiring(false);
        break;
    default:
        break;
    }
}

- (void)newGame {
    if (_game == nullptr) {
        return;
    }
    _game->startNewGame();
}

- (void)togglePause {
    if (_game != nullptr) {
        _game->togglePause();
    }
}

- (void)toggleMute {
    _muted = !_muted;
    _audio->setMuted(_muted == YES);
    [_renderer setMuted:_muted];
    [NSUserDefaults.standardUserDefaults setBool:_muted forKey:kMutedKey];
    if (!_muted) {
        _audio->play(invaders::platform::Sound::Start);
    }
}

- (BOOL)isMuted {
    return _muted;
}

- (void)saveHighScore {
    if (_game == nullptr) {
        return;
    }
    const int highScore = _game->highScore();
    if (highScore > _lastSavedHighScore) {
        [NSUserDefaults.standardUserDefaults setInteger:highScore forKey:kHighScoreKey];
        _lastSavedHighScore = highScore;
        _lastHighScoreSave = CACurrentMediaTime();
    }
}

- (void)shutdown {
    [self saveHighScore];
    [NSNotificationCenter.defaultCenter removeObserver:self];
    if (_game != nullptr) {
        _game->pause();
    }
    self.paused = YES;
    self.delegate = nil;
}

@end
