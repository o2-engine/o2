#import "AppDelegate.h"
#import <MetalKit/MetalKit.h>
#import "RendererView.h"
#include "o2/Utils/Math/Vector2.h"

@class RendererView;

namespace o2
{
    struct ApplicationPlatformWrapper
    {
        static RenderView*           view;
        static RenderViewController* viewController;
        static RendererViewDelegate* renderer;
        
        static Vec2I resolution;

        static void OnWindowResized(const Vec2I& resolution);
    };
}
