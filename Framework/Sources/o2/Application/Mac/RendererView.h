#ifdef PLATFORM_MAC

#import <MetalKit/MetalKit.h>

@interface RendererView : NSObject <MTKViewDelegate>

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;

@end

@interface ViewController : MTKView

- (void)initializeMouseTracking;

@end

#endif
