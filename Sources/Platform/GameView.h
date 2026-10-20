#pragma once

#import <MetalKit/MetalKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface SIGameView : MTKView <MTKViewDelegate>

- (nullable instancetype)initWithFrame:(NSRect)frame error:(NSError* _Nullable* _Nullable)error;

- (void)newGame;
- (void)togglePause;
- (void)toggleMute;
- (void)saveHighScore;
- (void)shutdown;

@property(nonatomic, readonly, getter=isMuted) BOOL muted;

@end

NS_ASSUME_NONNULL_END
