#ifdef PLATFORM_IOS
#import "AppDelegate.h"
#import <MetalKit/MetalKit.h>
#import "ApplicationPlatformWrapper.h"
#import "RendererView.h"
#include "o2/Render/Render.h"
#include "o2/Render/iOS/MetalWrappers.h"
#include "o2/Application/Application.h"
#include "o2/Utils/Debug/Debug.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

@synthesize window;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    
    o2::ApplicationPlatformWrapper::viewController = [[RenderViewController alloc] init];
    
    auto view = [[RenderView alloc] initWithFrame:window.bounds];
    o2::ApplicationPlatformWrapper::view = view;
    o2::ApplicationPlatformWrapper::viewController.view = view;
    view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    view.contentScaleFactor = UIScreen.mainScreen.nativeScale;
    
    view.colorPixelFormat = MTLPixelFormatRGBA8Unorm;
    view.device = MTLCreateSystemDefaultDevice();
    if(!view.device)
    {
        o2Debug.LogError("Metal is not supported on this device");
        return NO;
    }
    
    CGSize drawableSize = CGSizeMake(CGRectGetWidth(view.bounds) * view.contentScaleFactor,
                                     CGRectGetHeight(view.bounds) * view.contentScaleFactor);
    view.drawableSize = drawableSize;

    o2::ApplicationPlatformWrapper::renderer = [[RendererViewDelegate alloc] init];
    view.delegate = o2::ApplicationPlatformWrapper::renderer;
    
    [window setRootViewController:o2::ApplicationPlatformWrapper::viewController];
    [window makeKeyAndVisible];

    [view layoutIfNeeded];
    [o2::ApplicationPlatformWrapper::renderer mtkView:view drawableSizeWillChange:view.drawableSize];

    o2::ApplicationPlatformWrapper::InitializeApplication();
    return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
}


- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
}


- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

@end

#endif
