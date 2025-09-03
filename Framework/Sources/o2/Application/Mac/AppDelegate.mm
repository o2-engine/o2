#ifdef PLATFORM_MAC

#import "AppDelegate.h"
#import <MetalKit/MetalKit.h>
#import "RendererView.h"
#include "ApplicationPlatformWrapper.h"
#include "o2/Utils/Debug/Debug.h"

@implementation O2Application

- (void)sendEvent:(NSEvent *)event {
    if ([event type] == NSEventTypeKeyUp && ([event modifierFlags] & NSEventModifierFlagCommand)) {
        [[self keyWindow] sendEvent:event];
    } else {
        [super sendEvent:event];
    }
}

@end

@interface AppDelegate ()

@end

@implementation AppDelegate

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    o2::ApplicationPlatformWrapper::Deinitialize();
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

#endif
