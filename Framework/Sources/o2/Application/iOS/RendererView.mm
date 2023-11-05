#import "RendererView.h"

#ifdef PLATFORM_IOS

#import "ShaderTypes.h"
#include "o2/Render/Render.h"
#include "o2/Render/iOS/MetalWrappers.h"
#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Application/iOS/ApplicationPlatformWrapper.h"
#include "o2/Utils/Debug/Debug.h"

@implementation RendererViewDelegate

- (void)drawInMTKView:(nonnull MTKView *)view
{
    o2Application.Update();
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    o2::ApplicationPlatformWrapper::OnWindowRsized(o2::Vec2I(size.width, size.height));
}

@end

@implementation RenderViewController

@end

@implementation RenderView

- (o2::Vec2F)getTouchPosition:(UITouch *)touch
{
    o2::Vec2F res(([touch locationInView:self].x - self.bounds.size.width/2)*self.layer.contentsScale,
                  -([touch locationInView:self].y - self.bounds.size.height/2)*self.layer.contentsScale);
    
    return res;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent*) __unused event
{
    for (int i = 0; i < [touches count]; ++i)
    {
        UITouch *touch = [[touches allObjects] objectAtIndex:i];
        o2Input.OnCursorPressed([self getTouchPosition:touch], i);
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent*) __unused event
{
    for (int i = 0; i < [touches count]; ++i)
    {
        UITouch *touch = [[touches allObjects] objectAtIndex:i];
        o2Input.OnCursorMoved([self getTouchPosition:touch], i);
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent*) __unused event
{
    for (int i = 0; i < [touches count]; ++i)
        o2Input.OnCursorReleased(i);
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent*) __unused event
{
    for (int i = 0; i < [touches count]; ++i)
        o2Input.OnCursorReleased(i);
}

@end

#endif
