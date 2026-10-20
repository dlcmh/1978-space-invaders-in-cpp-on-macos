#import "MetalRenderer.h"

#include "MetalShaders.hpp"
#include "Scene.hpp"

#include "invaders/Game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

namespace {

struct Uniforms {
    float viewportWidth;
    float viewportHeight;
    float scaleX;
    float scaleY;
    float offsetX;
    float offsetY;
};

constexpr NSUInteger kInitialBufferBytes = 1U << 20U;
constexpr NSUInteger kBufferCount = 3;

NSError* makeError(NSString* description) {
    NSDictionary<NSString*, id>* userInfo = @{NSLocalizedDescriptionKey : description};
    return [NSError errorWithDomain:@"dev.dlcmh.space-invaders.renderer" code:1 userInfo:userInfo];
}

} // namespace

@implementation SIRenderer {
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
    id<MTLRenderPipelineState> _pipelineState;
    NSMutableArray<id<MTLBuffer>>* _vertexBuffers;
    std::unique_ptr<invaders::render::Scene> _scene;
    dispatch_semaphore_t _inFlightSemaphore;
    NSUInteger _frameIndex;
}

- (nullable instancetype)initWithView:(MTKView*)view error:(NSError* _Nullable* _Nullable)error {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _device = view.device;
    if (_device == nil) {
        if (error != nullptr) {
            *error = makeError(@"No Metal device is available.");
        }
        return nil;
    }

    _commandQueue = [_device newCommandQueue];
    if (_commandQueue == nil) {
        if (error != nullptr) {
            *error = makeError(@"Unable to create a Metal command queue.");
        }
        return nil;
    }

    NSString* source = [[NSString alloc] initWithBytes:invaders::render::kMetalShaderSource.data()
                                                length:invaders::render::kMetalShaderSource.size()
                                              encoding:NSUTF8StringEncoding];
    NSError* libraryError = nil;
    id<MTLLibrary> library = [_device newLibraryWithSource:source options:nil error:&libraryError];
    if (library == nil) {
        if (error != nullptr) {
            *error = libraryError != nil ? libraryError
                                         : makeError(@"Metal Shading Language compilation failed.");
        }
        return nil;
    }

    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"arcade_vertex"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"arcade_fragment"];
    if (vertexFunction == nil || fragmentFunction == nil) {
        if (error != nullptr) {
            *error = makeError(@"Metal shader entry points are missing.");
        }
        return nil;
    }

    MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
    descriptor.label = @"Space Invaders arcade pipeline";
    descriptor.vertexFunction = vertexFunction;
    descriptor.fragmentFunction = fragmentFunction;
    descriptor.rasterSampleCount = view.sampleCount;
    descriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    descriptor.colorAttachments[0].blendingEnabled = YES;
    descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    NSError* pipelineError = nil;
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:descriptor error:&pipelineError];
    if (_pipelineState == nil) {
        if (error != nullptr) {
            *error = pipelineError != nil
                         ? pipelineError
                         : makeError(@"Unable to create the Metal render pipeline.");
        }
        return nil;
    }

    _vertexBuffers = [NSMutableArray arrayWithCapacity:kBufferCount];
    for (NSUInteger index = 0; index < kBufferCount; ++index) {
        id<MTLBuffer> buffer = [_device newBufferWithLength:kInitialBufferBytes
                                                    options:MTLResourceStorageModeShared];
        if (buffer == nil) {
            if (error != nullptr) {
                *error = makeError(@"Unable to allocate a Metal vertex buffer.");
            }
            return nil;
        }
        [_vertexBuffers addObject:buffer];
    }

    _inFlightSemaphore = dispatch_semaphore_create(static_cast<long>(kBufferCount));
    _frameIndex = 0;
    _scene = std::make_unique<invaders::render::Scene>();
    return self;
}

- (void)renderGame:(const invaders::Game&)game
            inView:(MTKView*)view
      deltaSeconds:(float)deltaSeconds {
    _scene->update(deltaSeconds);
    _scene->build(game);

    const invaders::render::Canvas& canvas = _scene->canvas();
    if (canvas.empty()) {
        return;
    }

    id<CAMetalDrawable> drawable = [view currentDrawable];
    MTLRenderPassDescriptor* pass = [view currentRenderPassDescriptor];
    if (drawable == nil || pass == nil) {
        return;
    }

    pass.colorAttachments[0].clearColor = MTLClearColorMake(0.002, 0.006, 0.012, 1.0);

    const std::vector<float>& vertices = canvas.data();
    const NSUInteger byteCount = static_cast<NSUInteger>(vertices.size() * sizeof(float));

    dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);

    id<MTLBuffer> buffer = _vertexBuffers[_frameIndex];
    if (byteCount > buffer.length) {
        NSUInteger newLength =
            std::max<NSUInteger>(byteCount + (byteCount / 2U), kInitialBufferBytes);
        id<MTLBuffer> larger = [_device newBufferWithLength:newLength
                                                    options:MTLResourceStorageModeShared];
        if (larger == nil) {
            dispatch_semaphore_signal(_inFlightSemaphore);
            return;
        }
        _vertexBuffers[_frameIndex] = larger;
        buffer = larger;
    }
    std::memcpy(buffer.contents, vertices.data(), static_cast<std::size_t>(byteCount));
    _frameIndex = (_frameIndex + 1U) % kBufferCount;

    const CGSize drawableSize = view.drawableSize;
    if (drawableSize.width <= 0.0 || drawableSize.height <= 0.0) {
        dispatch_semaphore_signal(_inFlightSemaphore);
        return;
    }
    const float scale = std::min(static_cast<float>(drawableSize.width) / 224.0F,
                                 static_cast<float>(drawableSize.height) / 256.0F);
    const float offsetX = (static_cast<float>(drawableSize.width) - 224.0F * scale) * 0.5F;
    const float offsetY = (static_cast<float>(drawableSize.height) - 256.0F * scale) * 0.5F;
    Uniforms uniforms{static_cast<float>(drawableSize.width),
                      static_cast<float>(drawableSize.height),
                      scale,
                      scale,
                      offsetX,
                      offsetY};

    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    if (commandBuffer == nil) {
        dispatch_semaphore_signal(_inFlightSemaphore);
        return;
    }
    commandBuffer.label = @"Space Invaders frame";
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:pass];
    encoder.label = @"Arcade scene";
    [encoder setRenderPipelineState:_pipelineState];
    [encoder setVertexBuffer:buffer offset:0 atIndex:0];
    [encoder setVertexBytes:&uniforms length:sizeof(Uniforms) atIndex:1];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                vertexStart:0
                vertexCount:static_cast<NSUInteger>(canvas.vertexCount())];
    [encoder endEncoding];

    dispatch_semaphore_t semaphore = _inFlightSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> completedBuffer) {
      (void)completedBuffer;
      dispatch_semaphore_signal(semaphore);
    }];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

- (void)handleEvent:(const invaders::Event&)event {
    _scene->handleEvent(event);
}

- (void)setMuted:(BOOL)muted {
    _scene->setMuted(muted == YES);
}

@end
