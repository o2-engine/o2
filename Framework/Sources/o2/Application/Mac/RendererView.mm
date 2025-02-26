#ifdef PLATFORM_MAC

#import "RendererView.h"

#import "o2/Render/Mac/ShaderTypes.h"
#include "o2/Render/Render.h"
#include "o2/Render/Mac/MetalWrappers.h"
#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Application/Mac/ApplicationPlatformWrapper.h"
#include "o2/Utils/Debug/Debug.h"

@implementation RendererView

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;
{
    self = [super init];
    return self;
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    o2Application.Update();
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    o2::ApplicationPlatformWrapper::OnWindowRsized(o2::Vec2I(size.width, size.height));
}

@end

@implementation ViewController

- (void)updateTrackingAreas {
    [self initTrackingArea];
}

-(void)initTrackingArea {
    NSTrackingAreaOptions options = (NSTrackingActiveAlways | NSTrackingInVisibleRect |
                                     NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);
    
    NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                        options:options
                                                          owner:self
                                                       userInfo:nil];
    
    [self addTrackingArea:area];
}

- (int)getKeyCode:(NSEvent *)event
{
    const auto keyFlags = [event modifierFlags];
    if (!(keyFlags & NSEventModifierFlagFunction))
    {
        NSString *str = [event charactersIgnoringModifiers];
        if (str && [str length] != 0)
        {
            const int charCode = [str characterAtIndex : 0];
            if (charCode >= 0)
            {
                constexpr int keyBackspace = 0x7f;
                constexpr int keyReturn = 0x0d;
                constexpr int keyPadEnter = 0x03;
                constexpr int keyEscape = 0x1b;
                
                if ((charCode != keyBackspace) && (charCode != keyReturn) && (charCode != keyPadEnter) && (charCode != keyEscape))
                    return charCode;
            }
        }
    }
    
    return -[event keyCode];
}

- (void)keyDown:(NSEvent *)event
{
    o2Input.OnKeyPressed([self getKeyCode:event]);
}

- (void)keyUp:(NSEvent *)event
{
    o2Input.OnKeyReleased([self getKeyCode:event]);
}

- (void)flagsChanged:(NSEvent*)event
{
    bool shift = event.modifierFlags & NSShiftKeyMask;
    bool alt = event.modifierFlags & NSAlternateKeyMask;
    bool ctrl = event.modifierFlags & NSControlKeyMask;
    bool command = event.modifierFlags & NSCommandKeyMask;
    
    static bool prevShift = shift;
    static bool prevAlt = alt;
    static bool prevCtrl = ctrl;
    static bool prevCommand = command;
    
    if (shift != prevShift)
        shift ? o2Input.OnKeyPressed(VK_SHIFT) : o2Input.OnKeyReleased(VK_SHIFT);
    
    if (alt != prevAlt)
        alt ? o2Input.OnKeyPressed(VK_MENU) : o2Input.OnKeyReleased(VK_MENU);
    
    if (ctrl != prevCtrl)
        ctrl ? o2Input.OnKeyPressed(VK_CONTROL) : o2Input.OnKeyReleased(VK_CONTROL);
    
    if (command != prevCommand)
        command ? o2Input.OnKeyPressed(VK_COMMAND) : o2Input.OnKeyReleased(VK_COMMAND);
    
    prevShift = shift;
    prevAlt = alt;
    prevCtrl = ctrl;
    prevCommand = command;
}

- (o2::Vec2F)getMousePos:(NSEvent *)event
{
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    NSRect viewRectPoints = [self bounds];
    NSRect viewRectPixels = [self convertRectToBacking:viewRectPoints];
    
    float scale = o2Application.GetGraphicsScale();
    auto screenPoint = o2::Vec2F(o2::Math::Floor(pt.x*scale - viewRectPixels.size.width/2),
                                 o2::Math::Floor(pt.y*scale - viewRectPixels.size.height/2));
    
    return screenPoint;
}

- (void)mouseDown:(NSEvent *)event
{
    o2Input.OnCursorPressed([self getMousePos:event]);
}

- (void)mouseDragged:(NSEvent *)event
{
    o2Input.OnCursorMoved([self getMousePos:event]);
}

- (void)mouseMoved:(NSEvent *)event
{
    o2Input.OnCursorMoved([self getMousePos:event], 0);
}

- (void)mouseUp:(NSEvent *)event
{
    o2Input.OnCursorReleased();
}

- (void)rightMouseDown:(NSEvent *)event
{
    o2Input.OnAltCursorPressed([self getMousePos:event]);
}

- (void)rightMouseUp:(NSEvent *)event
{
    o2Input.OnAltCursorReleased();
}

- (void)scrollWheel:(NSEvent *)event
{
    o2Input.OnMouseWheel((float)event.scrollingDeltaY);
}

@end

#endif
