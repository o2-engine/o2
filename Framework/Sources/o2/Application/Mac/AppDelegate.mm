#ifdef PLATFORM_MAC

#import "AppDelegate.h"
#import <MetalKit/MetalKit.h>
#import "RendererView.h"
#include "ApplicationPlatformWrapper.h"

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
