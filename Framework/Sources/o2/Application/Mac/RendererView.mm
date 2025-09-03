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
    float scale = o2Application.GetGraphicsScale();
            o2::ApplicationPlatformWrapper::OnWindowResized(o2::Vec2I(size.width/scale, size.height/scale));
}

- (void)resetCursorRects
{
    [super resetCursorRects];
    [self addCursorRect:[self bounds] cursor:[NSCursor currentCursor]];
}

@end

@implementation ViewController

- (instancetype)init {
    self = [super init];
    if (self) {
        pressedKeysWithCmd = [[NSMutableSet alloc] init];
        currentlyPressedKeys = [[NSMutableSet alloc] init];
    }
    return self;
}

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        pressedKeysWithCmd = [[NSMutableSet alloc] init];
        currentlyPressedKeys = [[NSMutableSet alloc] init];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    if (self) {
        pressedKeysWithCmd = [[NSMutableSet alloc] init];
        currentlyPressedKeys = [[NSMutableSet alloc] init];
    }
    return self;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)canBecomeKeyView {
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent {
    return YES;
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    if (self.window) {
        [self.window makeFirstResponder:self];
        o2Debug.Log("View became first responder: %s", [self.window firstResponder] == self ? "YES" : "NO");
    }
}

- (void)awakeFromNib {
    [super awakeFromNib];
    if (self.window) {
        [self.window makeFirstResponder:self];
    }
}



- (void)initializeMouseTracking {
    [self initTrackingArea];
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    
    for (NSTrackingArea *area in [self trackingAreas]) {
        [self removeTrackingArea:area];
    }
    
    [self initTrackingArea];
}

-(void)initTrackingArea {
    NSRect bounds = [self bounds];
    
    NSTrackingAreaOptions options = (NSTrackingActiveAlways | NSTrackingInVisibleRect |
                                     NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);
    
    NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:bounds
                                                        options:options
                                                          owner:self
                                                       userInfo:nil];
    
    [self addTrackingArea:area];
}

- (void)resetCursorRects
{
    [super resetCursorRects];
    [self addCursorRect:[self bounds] cursor:[NSCursor currentCursor]];
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
    int keyCode = [self getKeyCode:event];
    bool cmdPressed = (event.modifierFlags & NSEventModifierFlagCommand) != 0;
    bool isRepeat = [event isARepeat];
    
    if (isRepeat || [currentlyPressedKeys containsObject:@(keyCode)]) {
        return;
    }
    
    [currentlyPressedKeys addObject:@(keyCode)];
    o2Input.OnKeyPressed(keyCode);
    
    if (cmdPressed) {
        [pressedKeysWithCmd addObject:@(keyCode)];
    }
}

- (void)keyUp:(NSEvent *)event
{
    int keyCode = [self getKeyCode:event];
    
    [currentlyPressedKeys removeObject:@(keyCode)];
    o2Input.OnKeyReleased(keyCode);
    
    [pressedKeysWithCmd removeObject:@(keyCode)];
}

- (void)flagsChanged:(NSEvent*)event
{
    bool shift = event.modifierFlags & NSEventModifierFlagShift;
    bool alt = event.modifierFlags & NSEventModifierFlagOption;
    bool ctrl = event.modifierFlags & NSEventModifierFlagControl;
    bool command = event.modifierFlags & NSEventModifierFlagCommand;
    
    static bool prevShift = false;
    static bool prevAlt = false;
    static bool prevCtrl = false;
    static bool prevCommand = false;
    
    if (shift != prevShift)
        shift ? o2Input.OnKeyPressed(VK_SHIFT) : o2Input.OnKeyReleased(VK_SHIFT);
    
    if (alt != prevAlt)
        alt ? o2Input.OnKeyPressed(VK_MENU) : o2Input.OnKeyReleased(VK_MENU);
    
    if (ctrl != prevCtrl)
        ctrl ? o2Input.OnKeyPressed(VK_CONTROL) : o2Input.OnKeyReleased(VK_CONTROL);
    
    if (command != prevCommand) {
        if (command) {
            o2Input.OnKeyPressed(VK_COMMAND);
        } else {
            o2Input.OnKeyReleased(VK_COMMAND);
            
            for (NSNumber *keyCodeNumber in pressedKeysWithCmd) {
                int keyCode = [keyCodeNumber intValue];
                o2Input.OnKeyReleased(keyCode);
                [currentlyPressedKeys removeObject:@(keyCode)];
            }
            [pressedKeysWithCmd removeAllObjects];
        }
    }
    
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
    auto screenPoint = o2::Vec2F(o2::Math::Floor(pt.x - viewRectPixels.size.width/2/scale),
                                 o2::Math::Floor(pt.y - viewRectPixels.size.height/2/scale));
    
    return screenPoint;
}

- (void)mouseDown:(NSEvent *)event
{
    o2Input.OnCursorPressed([self getMousePos:event]);
}

- (void)mouseDragged:(NSEvent *)event
{
    o2Input.OnCursorMoved([self getMousePos:event], 0);
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

- (void)rightMouseDragged:(NSEvent *)event
{
    o2Input.OnCursorMoved([self getMousePos:event], 0);
}

- (void)scrollWheel:(NSEvent *)event
{
    o2Input.OnMouseWheel((float)event.scrollingDeltaY);
}

@end

#endif
