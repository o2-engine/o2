#ifdef PLATFORM_MAC

#import "RendererView.h"

#import "o2/Render/Platforms/Mac/ShaderTypes.h"
#include "o2/Render/Render.h"
#include "o2/Render/Platforms/Mac/MetalWrappers.h"
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
    // The drawable is in pixels: a window moved to a screen of another density keeps its size in points while the
    // pixels per point change, so the scale is the one of the window's screen now, not the one of the start. A view not
    // in the window yet will open on the main screen
    float scale = view.window ? (float)view.window.backingScaleFactor : (float)[NSScreen mainScreen].backingScaleFactor;
    if (scale <= 0.0f)
        scale = o2Application.GetGraphicsScale();

    o2::ApplicationPlatformWrapper::OnWindowResized(o2::Vec2I(size.width/scale, size.height/scale), scale);
}

@end

static o2::MacKeyboardHandler::Modifiers GetModifiers(NSEventModifierFlags flags)
{
    o2::MacKeyboardHandler::Modifiers modifiers;
    modifiers.shift = (flags & NSEventModifierFlagShift) != 0;
    modifiers.alt = (flags & NSEventModifierFlagOption) != 0;
    modifiers.control = (flags & NSEventModifierFlagControl) != 0;
    modifiers.command = (flags & NSEventModifierFlagCommand) != 0;
    return modifiers;
}

@implementation ViewController

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)canBecomeKeyView {
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent {
    return YES;
}

- (void)viewDidChangeBackingProperties {
    [super viewDidChangeBackingProperties];

    // Another screen density: the drawable takes the pixels of the new screen, the delegate puts the new scale
    CGFloat scale = self.window ? self.window.backingScaleFactor : self.layer.contentsScale;
    self.layer.contentsScale = scale;

    CGSize pixels = CGSizeMake(self.bounds.size.width*scale, self.bounds.size.height*scale);
    if (!CGSizeEqualToSize(pixels, self.drawableSize))
        self.drawableSize = pixels;

    [self.delegate mtkView:self drawableSizeWillChange:self.drawableSize];
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidResignKeyNotification object:nil];
    if (self.window) {
        [self.window makeFirstResponder:self];
        o2Debug.Log("View became first responder: %s", [self.window firstResponder] == self ? "YES" : "NO");

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowDidResignKey:)
                                                     name:NSWindowDidResignKeyNotification
                                                   object:self.window];
    }
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
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

- (void)applyKeyEvents:(const o2::Vector<o2::MacKeyboardHandler::KeyEvent>&)events
{
    for (auto& event : events)
    {
        if (event.pressed)
            o2Input.OnKeyPressed(event.key);
        else
            o2Input.OnKeyReleased(event.key);
    }
}

- (void)keyDown:(NSEvent *)event
{
    [self applyKeyEvents:keyboardHandler.OnKeyDown([event keyCode], GetModifiers(event.modifierFlags))];
}

- (void)keyUp:(NSEvent *)event
{
    [self applyKeyEvents:keyboardHandler.OnKeyUp([event keyCode], GetModifiers(event.modifierFlags))];
}

- (void)flagsChanged:(NSEvent*)event
{
    [self applyKeyEvents:keyboardHandler.OnModifiersChanged(GetModifiers(event.modifierFlags))];
}

- (void)windowDidResignKey:(NSNotification *)notification
{
    [self applyKeyEvents:keyboardHandler.OnFocusLost()];
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
    [self applyKeyEvents:keyboardHandler.OnModifiersChanged(GetModifiers(event.modifierFlags))];
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
    [self applyKeyEvents:keyboardHandler.OnModifiersChanged(GetModifiers(event.modifierFlags))];
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

// The middle button comes as the "other" mouse button, number 2
- (void)otherMouseDown:(NSEvent *)event
{
    if (event.buttonNumber != 2)
        return;

    [self applyKeyEvents:keyboardHandler.OnModifiersChanged(GetModifiers(event.modifierFlags))];
    o2Input.OnAlt2CursorPressed([self getMousePos:event]);
}

- (void)otherMouseUp:(NSEvent *)event
{
    if (event.buttonNumber == 2)
        o2Input.OnAlt2CursorReleased();
}

- (void)otherMouseDragged:(NSEvent *)event
{
    o2Input.OnCursorMoved([self getMousePos:event], 0);
}

- (void)scrollWheel:(NSEvent *)event
{
    // touchpad and Magic Mouse report precise pixel deltas, a classic wheel reports
    // lines - the system flag tells them apart, no guessing by delta magnitude
    o2Input.OnMouseWheel(o2::Input::NormalizeWheelDelta((float)event.scrollingDeltaY,
                                                        event.hasPreciseScrollingDeltas));
}

@end

#endif
