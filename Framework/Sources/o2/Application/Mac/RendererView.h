#ifdef PLATFORM_MAC

#import <MetalKit/MetalKit.h>

#include "o2/Application/Mac/MacKeyboard.h"

@interface RendererView : NSObject <MTKViewDelegate>

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;

@end

@interface ViewController : MTKView {
    o2::MacKeyboardHandler keyboardHandler;
}

- (void)initializeMouseTracking;

@end

#endif
