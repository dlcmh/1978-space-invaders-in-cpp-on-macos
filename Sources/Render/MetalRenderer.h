#pragma once

#ifdef __cplusplus
#include <memory>

namespace invaders {
class Game;
struct Event;
} // namespace invaders

namespace invaders::render {
class Scene;
}
#endif

#import <Foundation/Foundation.h>
#import <MetalKit/MetalKit.h>

NS_ASSUME_NONNULL_BEGIN

/// The thin Objective-C++ bridge between the portable C++ scene and Apple's
/// Metal/AppKit APIs. Metal's resources are Objective-C protocols, so this one
/// boundary is Objective-C++; the simulation and scene remain clean C++.
@interface SIRenderer : NSObject

- (nullable instancetype)initWithView:(MTKView*)view error:(NSError* _Nullable* _Nullable)error;
- (void)renderGame:(const invaders::Game&)game
            inView:(MTKView*)view
      deltaSeconds:(float)deltaSeconds;
- (void)handleEvent:(const invaders::Event&)event;
- (void)setMuted:(BOOL)muted;

@end

NS_ASSUME_NONNULL_END
