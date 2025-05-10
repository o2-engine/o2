#ifdef PLATFORM_MAC

#import "AppDelegate.h"
#import <MetalKit/MetalKit.h>
#include "o2/Utils/Math/Vector2.h"

@class RendererView;

namespace o2
{
    struct ApplicationPlatformWrapper
    {
        static NSWindow*     window;
        static MTKView*      view;
        static RendererView* renderer;
        
        static Vec2I resolution;

        static void OnWindowRsized(const Vec2I& resolution);
        static void Deinitialize();
    };
}

#endif
