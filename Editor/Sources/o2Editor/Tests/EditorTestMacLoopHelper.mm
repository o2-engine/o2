#if defined(PLATFORM_MAC)

#include "EditorTestMacLoopHelper.h"

#import <Foundation/Foundation.h>

namespace Editor::Tests
{
    void StartTestTickTimer(TickCallback cb)
    {
        if (!cb)
            return;

        NSTimer* timer = [NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0
                                                         repeats:YES
                                                           block:^(NSTimer* t) {
            (void)t;
            cb(1.0f / 60.0f);
        }];
        [[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
    }
}

#endif
