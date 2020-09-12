#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <CoreFoundation/CoreFoundation.h>

void MacCapsLockSet(int on)
{
    kern_return_t kr;
    io_service_t ios;
    io_connect_t ioc;
    CFMutableDictionaryRef mdict;
    
    mdict = IOServiceMatching(kIOHIDSystemClass);
    ios = IOServiceGetMatchingService(kIOMasterPortDefault, (CFDictionaryRef) mdict);
    if (!ios) {
        if (mdict)
            CFRelease(mdict);
        return;
    }

    kr = IOServiceOpen(ios, mach_task_self(), kIOHIDParamConnectType, &ioc);
    IOObjectRelease(ios);
    if (kr != KERN_SUCCESS) {
        return;
    }

    IOHIDSetModifierLockState(ioc, kIOHIDCapsLockState, on);
    IOServiceClose(ioc);
}
