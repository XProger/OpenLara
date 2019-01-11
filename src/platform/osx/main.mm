#include <Cocoa/Cocoa.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <IOKit/hid/IOHidLib.h>

#include "game.h"

// timing
int osGetTime() {
    static mach_timebase_info_data_t timebaseInfo;
    if (timebaseInfo.denom == 0) {
        mach_timebase_info(&timebaseInfo);
    }
    
    uint64_t absolute = mach_absolute_time();
    uint64_t milliseconds = absolute * timebaseInfo.numer / (timebaseInfo.denom * 1000000ULL);
    return int(milliseconds);
}

// sound
#define SND_SIZE 2352

static AudioQueueRef audioQueue;

void sndFill(void* inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer) {
    void* frames = inBuffer->mAudioData;
    UInt32 count = inBuffer->mAudioDataBytesCapacity / 4;
    Sound::fill((Sound::Frame*)frames, count);
    inBuffer->mAudioDataByteSize = count * 4;
    AudioQueueEnqueueBuffer(audioQueue, inBuffer, 0, NULL);
}

void sndInit() {
    AudioStreamBasicDescription deviceFormat;
    deviceFormat.mSampleRate        = 44100;
    deviceFormat.mFormatID          = kAudioFormatLinearPCM;
    deviceFormat.mFormatFlags       = kLinearPCMFormatFlagIsSignedInteger;
    deviceFormat.mBytesPerPacket    = 4;
    deviceFormat.mFramesPerPacket   = 1;
    deviceFormat.mBytesPerFrame     = 4;
    deviceFormat.mChannelsPerFrame  = 2;
    deviceFormat.mBitsPerChannel    = 16;
    deviceFormat.mReserved          = 0;
    
    AudioQueueNewOutput(&deviceFormat, sndFill, NULL, NULL, NULL, 0, &audioQueue);
    
    for (int i = 0; i < 2; i++) {
        AudioQueueBufferRef mBuffer;
        AudioQueueAllocateBuffer(audioQueue, SND_SIZE, &mBuffer);
        sndFill(NULL, audioQueue, mBuffer);
    }
    AudioQueueStart(audioQueue, NULL);
}

// common input functions
InputKey keyToInputKey(int code) {
    static const int codes[] = {
        0x7B, 0x7C, 0x7E, 0x7D, 0x31, 0x30, 0x24, 0x35, 0x38, 0x3B, 0x3A,
        0x1D, 0x12, 0x13, 0x14, 0x15, 0x17, 0x16, 0x1A, 0x1C, 0x19,                   // 0..9
        0x00, 0x0B, 0x08, 0x02, 0x0E, 0x03, 0x05, 0x04, 0x22, 0x26, 0x28, 0x25, 0x2E, // A..M
        0x2D, 0x1F, 0x23, 0x0C, 0x0F, 0x01, 0x11, 0x20, 0x09, 0x0D, 0x07, 0x10, 0x06, // N..Z
    };

    for (int i = 0; i < sizeof(codes) / sizeof(codes[0]); i++)
        if (codes[i] == code) {
            return (InputKey)(ikLeft + i);
        }
    return ikNone;
}

InputKey mouseToInputKey(int btn) {
    switch (btn) {
        case 1 : return ikMouseL;
        case 2 : return ikMouseR;
        case 3 : return ikMouseM;
    }
    return ikNone;
}

IOHIDDeviceRef  joyDevices[4] = { 0 };
IOHIDManagerRef hidManager;

JoyKey joyButtonToKey(uint32_t button) {
    static const JoyKey keys[] = { jkA, jkB, jkX, jkY, jkLB, jkRB, jkLT, jkRT, jkStart, jkSelect, jkNone, jkUp, jkDown, jkLeft, jkRight };
    
    if (button >= 0 || button < COUNT(keys))
        return keys[button];
    return jkNone;
}

bool osJoyReady(int index) {
    if (index < 0 || index >= COUNT(joyDevices))
        return false;
    return joyDevices[index] != NULL;
}

void osJoyVibrate(int index, float L, float R) {
    // TODO
}

float joyAxisValue(IOHIDValueRef value) {
    IOHIDElementRef element = IOHIDValueGetElement(value);
    CFIndex val = IOHIDValueGetIntegerValue(value);
    CFIndex min = IOHIDElementGetLogicalMin(element);
    CFIndex max = IOHIDElementGetLogicalMax(element);
    
    float v = float(val - min) / float(max - min);
    if (v < 0.2f) v = 0.0f; // check for deadzone
    return v * 2.0f - 1.0f;
}

void hidValueCallback(void *context, IOReturn result, void *sender, IOHIDValueRef value) {
    if (result != kIOReturnSuccess)
        return;

    IOHIDElementRef element = IOHIDValueGetElement(value);
    IOHIDDeviceRef  device  = IOHIDElementGetDevice(element);
    
    int joyIndex = -1;
    for (int i = 0; i < COUNT(joyDevices); i++) {
        if (joyDevices[i] == device) {
            joyIndex = i;
            break;
        }
    }
    
    if (joyIndex == -1) return;

    // TODO: add mapping for most popular gamepads by kIOHIDVendorIDKey / kIOHIDProductIDKey
    switch (IOHIDElementGetUsagePage(element)) {
        case kHIDPage_GenericDesktop : {
            switch (IOHIDElementGetUsage(IOHIDValueGetElement(value))) {
                case kHIDUsage_GD_X  :
                    Input::setJoyPos(joyIndex, jkL, vec2(joyAxisValue(value), Input::joy[joyIndex].L.y));
                    break;
                case kHIDUsage_GD_Y  :
                    Input::setJoyPos(joyIndex, jkL, vec2(Input::joy[joyIndex].L.x, joyAxisValue(value)));
                    break;
                case kHIDUsage_GD_Rx :
                    Input::setJoyPos(joyIndex, jkR, vec2(joyAxisValue(value), Input::joy[joyIndex].R.y));
                    break;
                case kHIDUsage_GD_Ry :
                    Input::setJoyPos(joyIndex, jkR, vec2(Input::joy[joyIndex].R.x, joyAxisValue(value)));
                    break;
                case kHIDUsage_GD_Z  :
                    Input::setJoyPos(joyIndex, jkLT, vec2(joyAxisValue(value) * 0.5f + 0.5f, 0.0f));
                    break;
                case kHIDUsage_GD_Rz :
                    Input::setJoyPos(joyIndex, jkRT, vec2(joyAxisValue(value) * 0.5f + 0.5f, 0.0f));
                    break;
                default : LOG("! joy: unknown joy 0x%x (%d)\n", IOHIDElementGetUsage(IOHIDValueGetElement(value)), (int)IOHIDValueGetIntegerValue(value));
            }
            break;
        }
        case kHIDPage_Button : {
            uint32_t button = IOHIDElementGetUsage(IOHIDValueGetElement(value)) - kHIDUsage_Button_1;
            bool down  = IOHIDValueGetIntegerValue(value) != 0;
            JoyKey key = joyButtonToKey(button);
            Input::setJoyDown(joyIndex, key, down);
            if (key == jkNone) LOG("! joy: unknown button %d\n", button);
            break;
        }
        default : ;
    }
}

void joyAdd(void* context, IOReturn, void*, IOHIDDeviceRef device) {
    for (int i = 0; i < COUNT(joyDevices); i++) {
        if (joyDevices[i] == NULL) {
            joyDevices[i] = device;
            break;
        }
    }
}

void joyRemove(void* context, IOReturn, void*, IOHIDDeviceRef device) {
    for (int i = 0; i < COUNT(joyDevices); i++) {
        if (joyDevices[i] == device) {
            joyDevices[i] = NULL;
            break;
        }
    }
}

void joyInit() {
    hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDManagerOptionNone);
    
    NSDictionary *matchingGamepad = @{
                                      @(kIOHIDDeviceUsagePageKey): @(kHIDPage_GenericDesktop),
                                      @(kIOHIDDeviceUsageKey): @(kHIDUsage_GD_GamePad)
                                      };
    NSArray *matchDicts = @[ matchingGamepad ];
    
    IOHIDManagerSetDeviceMatchingMultiple(hidManager, (__bridge CFArrayRef) matchDicts);
    IOHIDManagerRegisterDeviceMatchingCallback(hidManager, joyAdd, NULL);
    IOHIDManagerRegisterDeviceRemovalCallback(hidManager, joyRemove, NULL);
    IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
    IOHIDManagerRegisterInputValueCallback(hidManager, hidValueCallback, NULL);
}

void joyFree() {
    IOHIDManagerUnscheduleFromRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDManagerRegisterDeviceMatchingCallback(hidManager, NULL, 0);
    IOHIDManagerRegisterDeviceRemovalCallback(hidManager, NULL, 0);
    IOHIDManagerClose(hidManager, kIOHIDOptionsTypeNone);
}

@interface OpenLaraGLView : NSOpenGLView

@end

@implementation OpenLaraGLView

- (InputKey)inputKeyForMouseEvent:(NSEvent *)theEvent {
    switch (theEvent.buttonNumber) {
        case 0  : return ikMouseL;
        case 1  : return ikMouseR;
        case 2  : return ikMouseM;
        default : return ikNone;
    }
}

- (vec2)inputPositionForMouseEvent:(NSEvent *)theEvent {
    NSPoint inWindow = theEvent.locationInWindow;
    NSPoint inView = [self convertPoint:inWindow fromView:nil];
    return vec2(inView.x, Core::height - inView.y);
}

- (void)mouseDown:(NSEvent *)theEvent {
    InputKey inputKey = [self inputKeyForMouseEvent:theEvent];
    Input::setPos(inputKey, [self inputPositionForMouseEvent:theEvent]);
    Input::setDown(inputKey, true);
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    [self mouseDown:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent {
    [self mouseDown:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent {
    InputKey inputKey = [self inputKeyForMouseEvent:theEvent];
    Input::setPos(inputKey, [self inputPositionForMouseEvent:theEvent]);
    Input::setDown(inputKey, false);
}

- (void)rightMouseUp:(NSEvent *)theEvent {
    [self mouseUp:theEvent];
}

- (void)otherMouseUp:(NSEvent *)theEvent {
    [self mouseUp:theEvent];
}

- (void)mouseDragged:(NSEvent *)theEvent {
    InputKey inputKey = [self inputKeyForMouseEvent:theEvent];
    Input::setPos(inputKey, [self inputPositionForMouseEvent:theEvent]);
}

- (void)rightMouseDragged:(NSEvent *)theEvent {
    [self mouseDragged:theEvent];
}

- (void)otherMouseDragged:(NSEvent *)theEvent {
    [self mouseDragged:theEvent];
}

- (void)keyDown:(NSEvent *)theEvent {
    unsigned short keyCode = theEvent.keyCode;
    Input::setDown(keyToInputKey(keyCode), true);
}

- (void)keyUp:(NSEvent *)theEvent {
    unsigned short keyCode = theEvent.keyCode;
    Input::setDown(keyToInputKey(keyCode), false);
}

- (void)flagsChanged:(NSEvent *)theEvent {
    NSEventModifierFlags modifiers = theEvent.modifierFlags;
    Input::setDown(ikShift, modifiers & NSEventModifierFlagShift);
    Input::setDown(ikCtrl,  modifiers & NSEventModifierFlagControl);
    Input::setDown(ikAlt,   modifiers & NSEventModifierFlagOption);
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)reshape {
    NSRect bounds = self.bounds;
    Core::width  = bounds.size.width;
    Core::height = bounds.size.height;
}

- (void)prepareOpenGL {
    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
}

- (void)drawRect:(NSRect)dirtyRect {
    NSOpenGLContext *context = [self openGLContext];
    
    if (!Game::update())
        return;
    Game::render();

    [context flushBuffer];
    
    BOOL arg = YES;
    NSInvocation *inv = [NSInvocation invocationWithMethodSignature:[self methodSignatureForSelector:@selector(setNeedsDisplay:)]];
    [inv setSelector:@selector(setNeedsDisplay:)];
    [inv setTarget:self];
    [inv setArgument:&arg atIndex:2];
    [inv performSelector:@selector(invoke) withObject:self afterDelay:0.01];
}

@end

@interface OpenLaraWindowDelegate : NSObject<NSWindowDelegate>
@end

@implementation OpenLaraWindowDelegate

- (void)windowWillClose:(NSNotification *)notification {
    [[NSApplication sharedApplication] terminate:self];
}

@end

int main() {
    cacheDir[0] = saveDir[0] = contentDir[0] = 0;
    
    NSApplication *application = [NSApplication sharedApplication];
    
    // init window
    NSRect rect = NSMakeRect(0, 0, 1280, 720);
    NSWindow *window = [[NSWindow alloc] initWithContentRect:rect styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask backing:NSBackingStoreBuffered defer:YES];
    window.title = @"OpenLara";
    window.acceptsMouseMovedEvents = YES;
    window.delegate = [[OpenLaraWindowDelegate alloc] init];
    
    // init OpenGL context
    NSOpenGLPixelFormatAttribute attribs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize,     32,
        NSOpenGLPFADepthSize,     24,
        NSOpenGLPFAStencilSize,   8,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
        0
    };
    NSOpenGLPixelFormat *format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];

    OpenLaraGLView *view = [[OpenLaraGLView alloc] initWithFrame:window.contentLayoutRect pixelFormat:format];
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    window.contentView = view;
    [view.openGLContext makeCurrentContext];
    
    // get path to game content
    NSBundle *bundle   = [NSBundle mainBundle];
    NSURL *resourceURL = bundle.resourceURL;
    [resourceURL getFileSystemRepresentation:contentDir maxLength:sizeof(contentDir)];
    strcat(contentDir, "/");

    // show window
    [window center];
    [window makeKeyAndOrderFront:nil];

    joyInit();
    sndInit();
    Game::init();

    if (!Core::isQuit) {
        [application run];
    }
    
    Game::deinit();
    joyFree();
    // TODO: sndFree

    return 0;
}
