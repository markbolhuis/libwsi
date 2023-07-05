#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "utils.h"

#include "common_priv.h"
#include "platform_priv.h"
#include "window_priv.h"

//=====================Cocoa OBJECTS=====================
static const NSRange nsEmptyRange = { NSNotFound, 0 };

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

@interface CocoaWindowDelegate : NSObject {
    WsiWindow window;
    bool initFocusFinished;
}

- (instancetype)initWithWSIWindow:(WsiWindow)initWindow;

@end

@implementation CocoaWindowDelegate

- (instancetype)initWithWSIWindow:(WsiWindow)initWindow {
    self = [super init];
    if (self != nil) {
        window = initWindow;
        initFocusFinished = false;
    }

    return self;
}

- (BOOL)windowShouldClose:(id)sender {
    WsiCloseWindowEvent info = {
        .base.type = WSI_EVENT_TYPE_CLOSE_WINDOW,
        .base.flags = 0,
        .base.serial = 0,//event->sequence, //TODO: find out what this is
        .base.time = 0,
        .window = window,
    };

    window->pfn_close(window->user_data, &info);

    return NO;
}

- (void)windowDidResize:(NSNotification *)notification {
    const NSRect contentRect = [window->view frame];
    const NSRect fbRect = [window->view convertRectToBacking:contentRect];

    //TODO
}

- (void)windowDidMove:(NSNotification*)notification {
    
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
    
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    
}

- (void)windowDidResignKey:(NSNotification*)notification {
    
}

- (void)windowDidChangeOcclusionState:(NSNotification*)notification {
    
}

@end

//View

@interface CocoaContentView : NSView <NSTextInputClient> {
    WsiWindow window;
    NSTrackingArea* trackingArea;
    NSMutableAttributedString* markedText;
}

- (instancetype)initWithWSIWindow:(WsiWindow)initWindow;

@end

@implementation CocoaContentView

- (instancetype)initWithWSIWindow:(WsiWindow)initWindow {
    self = [super init];
    if (self != nil) {
        window = initWindow;
        trackingArea = nil;
        markedText = [[NSMutableAttributedString alloc] init];

        [self updateTrackingAreas];
        [self registerForDraggedTypes:@[NSPasteboardTypeURL]];
    }

    return self;
}

- (void)dealloc {
    [trackingArea release];
    [markedText release];
    [super dealloc];
    [super release];
}

- (BOOL)isOpaque {
    return YES;
}

- (BOOL)canBecomeKeyView {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)wantsUpdateLayer {
    return YES;
}

- (void)updateLayer {
    
}

- (void)cursorUpdate:(NSEvent*)event {
    
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
    return YES;
}

- (void)mouseDown:(NSEvent*)event {
    //TODO
}

- (void)mouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)mouseUp:(NSEvent*)event {
    //TODO
}

- (void)mouseMoved:(NSEvent*)event {
    //TODO
}

- (void)rightMouseDown:(NSEvent*)event {
    //TODO
}

- (void)rightMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)rightMouseUp:(NSEvent*)event {
    //TODO
}

- (void)otherMouseDown:(NSEvent*)event {
    
}

- (void)otherMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)otherMouseUp:(NSEvent*)event {
    
}

- (void)mouseExited:(NSEvent*)event {
    //TODO
}

- (void)mouseEntered:(NSEvent*)event {
    //TODO
}

- (void)viewDidChangeBackingProperties {
    const NSRect contentRect = [window->view frame];
    const NSRect fbRect = [window->view convertRectToBacking:contentRect];
    //TODO
}

- (void)drawRect:(NSRect)rect {
    
}

- (void)updateTrackingAreas {
    if (trackingArea != nil) {
        [self removeTrackingArea:trackingArea];
        [trackingArea release];
    }

    const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
                                          NSTrackingActiveInKeyWindow |
                                          NSTrackingEnabledDuringMouseDrag |
                                          NSTrackingCursorUpdate |
                                          NSTrackingInVisibleRect |
                                          NSTrackingAssumeInside;

    trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                options:options
                                                  owner:self
                                               userInfo:nil];

    [self addTrackingArea:trackingArea];
    [super updateTrackingAreas];
}

- (void)keyDown:(NSEvent *)event {
    //TODO
}

- (void)flagsChanged:(NSEvent *)event {
    //TODO
}

- (void)keyUp:(NSEvent*)event {
    //TODO
}

- (void)scrollWheel:(NSEvent*)event {
    //TODO
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender {
    return NSDragOperationGeneric;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender {
    return YES;
}

- (BOOL)hasMarkedText {
    return [markedText length] > 0;
}

- (NSRange)markedRange {
    if ([markedText length] > 0)
        return NSMakeRange(0, [markedText length] - 1);
    else
        return nsEmptyRange;
}

- (NSRange)selectedRange {
    return nsEmptyRange;
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange {
    [markedText release];
    if ([string isKindOfClass:[NSAttributedString class]])
        markedText = [[NSMutableAttributedString alloc] initWithAttributedString:string];
    else
        markedText = [[NSMutableAttributedString alloc] initWithString:string];
}

- (void)unmarkText {
    [[markedText mutableString] setString:@""];
}

- (NSArray*)validAttributesForMarkedText {
    return [NSArray array];
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:(NSRangePointer)actualRange {
    return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range
                         actualRange:(NSRangePointer)actualRange {
    const NSRect frame = [window->view frame];
    return NSMakeRect(frame.origin.x, frame.origin.y, 0.0, 0.0);
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    //TODO
}

- (void)doCommandBySelector:(SEL)selector {

}

@end

//Window Object

@interface CocoaWindow : NSWindow {}
@end

@implementation CocoaWindow

- (BOOL)canBecomeKeyWindow {
    return YES;
}

- (BOOL)canBecomeMainWindow {
    return YES;
}

@end

WsiResult
wsiCreateWindow(
    WsiPlatform platform,
    const WsiWindowCreateInfo *pCreateInfo,
    WsiWindow *pWindow)
{
    struct wsi_window *window = calloc(1, sizeof(struct wsi_window));
    if (!window) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    window->platform = platform;
    window->api = WSI_API_NONE;
    window->user_width = wsi_xcb_clamp(pCreateInfo->extent.width);
    window->user_height = wsi_xcb_clamp(pCreateInfo->extent.height);
    window->user_data = pCreateInfo->pUserData;
    window->pfn_configure = pCreateInfo->pfnConfigureWindow;
    window->pfn_close = pCreateInfo->pfnCloseWindow;

    //=====================Helper=====================
    CocoaHelper* helper = [[CocoaHelper alloc] init];
    
    [NSThread detachNewThreadSelector:@selector(doNothing:)
                             toTarget:helper
                           withObject:nil];

    [NSApplication sharedApplication];

    //=====================App delegate=====================
    id appDelegate = [[CocoaAppDelegate alloc] init];
    [NSApp setDelegate:appDelegate];

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

    //=====================Window delegate=====================
    window->windowDelegate = [[CocoaWindowDelegate alloc] initWithWSIWindow:window];
    //if (windowDelegate == NULL) {
    //    LVND_ERROR("Failed to create NS window delegate");
    //}

    NSRect contentRect = NSMakeRect(0, 0, window->user_width, window->user_height);

    NSUInteger styleMask = NSWindowStyleMaskMiniaturizable;
    styleMask |= (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable);
    styleMask |= NSWindowStyleMaskResizable;
    window->window = [[CocoaWindow alloc]
        initWithContentRect:contentRect
                  styleMask:styleMask
                    backing:NSBackingStoreBuffered
                      defer:NO];
    //if (window == NULL) {
    //    LVND_ERROR("Failed to create NS window");
    //}
    
    [(NSWindow*)window->window center];

    const NSWindowCollectionBehavior behavior =
        NSWindowCollectionBehaviorFullScreenPrimary |
        NSWindowCollectionBehaviorManaged;
    [window->window setCollectionBehavior:behavior];

    window->view = [[CocoaContentView alloc] initWithWSIWindow:window];
    //if (view == NULL) {
    //    LVND_ERROR("Failed to create NS view");
    //}

    [window->window setContentView:window->view];
    [window->window makeFirstResponder:window->view];
    //[window->window setTitle:@(title)];
    [window->window setDelegate:window->windowDelegate];
    [window->window setAcceptsMouseMovedEvents:YES];
    [window->window setRestorable:NO];

    [NSApp activateIgnoringOtherApps:YES];
    [window->window makeKeyAndOrderFront:window->view];

    //Getting some properties
    /*
    const NSRect fbRect = [(id)window->handle->view convertRectToBacking:contentRect];
    window->framebufferWidth = fbRect.size.width;
    window->framebufferHeight = fbRect.size.height;

    //Retina
    if (window->width == window->framebufferWidth && window->height == window->framebufferHeight)
        window->handle->isRetina = false;
    else
        window->handle->isRetina = true;
    
    const NSPoint mousePos = [(id)window->handle->window mouseLocationOutsideOfEventStream];
    window->mouseX = mousePos.x;
    window->mouseY = mousePos.y;
    */

    //[app setDelegate:handle->delegate];

    //[(id)window->handle->window orderFrontRegardless];
    [NSApp run];

    *pWindow = window;

    return WSI_SUCCESS;
}

void
wsiDestroyWindow(WsiWindow window)
{
    //TODO
}

WsiResult
wsiSetWindowParent(WsiWindow window, WsiWindow parent)
{
    //TODO

    return WSI_ERROR_UNSUPPORTED;
}

WsiResult
wsiSetWindowTitle(WsiWindow window, const char *pTitle)
{
    [window->window setTitle:@(pTitle)];

    return WSI_ERROR_UNSUPPORTED;
}
