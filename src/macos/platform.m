#include <stdlib.h>
#include <string.h>

#include "common_priv.h"
#include "platform_priv.h"
#include "window_priv.h"

//=====================Cocoa OBJECTS=====================
@interface CocoaAppDelegate : NSObject <NSApplicationDelegate> {
    
}

- (void) menuItemClicked:(id) sender;

@end

@implementation CocoaAppDelegate

- (void) menuItemClicked:(id) sender {
    
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    return NSTerminateCancel;
}

- (void)applicationDidChangeScreenParameters:(NSNotification *) notification {
    
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
    
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [NSApp stop:nil];
}

- (void)applicationDidHide:(NSNotification *)notification {
    
}

@end

@interface CocoaHelper : NSObject
@end

@implementation CocoaHelper

- (void)selectedKeyboardInputSourceChanged:(NSObject* )object {
    
}

- (void)doNothing:(id)object {
}

@end

static WsiResult
wsi_platform_init(const WsiPlatformCreateInfo *pCreateInfo, struct wsi_platform *platform)
{
    //=====================Helper=====================
    CocoaHelper* helper = [[CocoaHelper alloc] init];
    
    [NSThread detachNewThreadSelector:@selector(doNothing:)
                             toTarget:helper
                           withObject:nil];

    [NSApplication sharedApplication];

    //=====================App delegate=====================
    platform->appDelegate = [[CocoaAppDelegate alloc] init];
    [NSApp setDelegate:platform->appDelegate];

    NSEvent* (^block)(NSEvent*) = ^ NSEvent* (NSEvent* event) {
        if ([event modifierFlags] & NSEventModifierFlagCommand)
            [[NSApp keyWindow] sendEvent:event];

        return event;
    };

    [[NSNotificationCenter defaultCenter]
        addObserver:helper
           selector:@selector(selectedKeyboardInputSourceChanged:)
               name:NSTextInputContextKeyboardSelectionDidChangeNotification
             object:nil];
    
    CGEventSourceSetLocalEventsSuppressionInterval(CGEventSourceCreate(kCGEventSourceStateHIDSystemState), 0.0);

    //if (![[NSRunningApplication currentApplication] isFinishedLaunching])
    //    [NSApp run];

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    return WSI_SUCCESS;
}

static void
wsi_platform_uninit(struct wsi_platform *platform)
{
    [platform->appDelegate release];
}

WsiResult
wsiCreatePlatform(const WsiPlatformCreateInfo *pCreateInfo, WsiPlatform *pPlatform)
{
    struct wsi_platform *p = calloc(1, sizeof(struct wsi_platform));
    if (!p) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    WsiResult result = wsi_platform_init(pCreateInfo, p);
    if (result != WSI_SUCCESS) {
        free(p);
        return result;
    }

    *pPlatform = p;
    return WSI_SUCCESS;
}

void
wsiDestroyPlatform(WsiPlatform platform)
{
    wsi_platform_uninit(platform);
    free(platform);
}

WsiResult
wsiDispatchEvents(WsiPlatform platform, int64_t timeout)
{
    @autoreleasepool {
    
    for (;;) {
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate:[NSDate distantPast]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
        if (event == nil)
            break;

        [NSApp sendEvent:event];
    }

    } // autoreleasepool

    return WSI_SUCCESS;
}
