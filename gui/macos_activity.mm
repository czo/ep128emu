#import <Foundation/Foundation.h>
#include "macos_activity.hpp"

void *ep128emuBeginMacOSRealtimeActivity()
{
  @autoreleasepool {
    NSActivityOptions options =
        NSActivityUserInitiatedAllowingIdleSystemSleep |
        NSActivityLatencyCritical;
    id token = [[NSProcessInfo processInfo]
        beginActivityWithOptions:options
        reason:@"ep128emu real-time emulation and audio"];
#if !__has_feature(objc_arc)
    [token retain];
#endif
    return (void *) token;
  }
}

void ep128emuEndMacOSRealtimeActivity(void *activityToken)
{
  if (!activityToken)
    return;
  @autoreleasepool {
    id token = (id) activityToken;
    [[NSProcessInfo processInfo] endActivity:token];
#if !__has_feature(objc_arc)
    [token release];
#endif
  }
}
