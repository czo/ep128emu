#ifndef EP128EMU_MACOS_ACTIVITY_HPP
#define EP128EMU_MACOS_ACTIVITY_HPP

#ifdef __APPLE__
void *ep128emuBeginMacOSRealtimeActivity();
void ep128emuEndMacOSRealtimeActivity(void *activityToken);
#endif

#endif
