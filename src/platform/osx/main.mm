#include <Cocoa/Cocoa.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <IOKit/hid/IOHidLib.h>

#include "game.h"

int MSAA = 0;
BOOL enableRetina = YES;
int frameLimit = 58 + 4;

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

    for (int i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
        if (codes[i] == code) {
            return (InputKey)(ikLeft + i);
        }
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
    
    float v = float(val - min) / float(max - min) * 2.0f - 1.0f;
    if (v > -0.2f && v < 0.2f) v = 0.0f; // check for deadzone
    return v;
}

static const struct { uint32 vendorId, productId; } JOY_VENDORS[] = {
    { 0x045E, 0x0000 }, // Microsoft
    { 0x2DC8, 0x0000 }, // 8Bitdo
    { 0x054C, 0x05C4 }, // Sony DS4 CUH-ZCT1x
    { 0x054C, 0x09CC }, // Sony DS4 CUH-ZCT2x
    { 0x054C, 0x0000 }, // Sony DS3
    { 0x2717, 0x0000 }, // Xiaomi
} ;

#define JOY_MAX_VENDORS COUNT(JOY_VENDORS)
#define JOY_MAX_BUTTONS 17

static const uint32 joyAxisTable[][6] = {
    { kHIDUsage_GD_X, kHIDUsage_GD_Y, kHIDUsage_GD_Rx, kHIDUsage_GD_Ry, kHIDUsage_GD_Z, kHIDUsage_GD_Rz },
    { kHIDUsage_GD_X, kHIDUsage_GD_Y, kHIDUsage_GD_Z,  kHIDUsage_GD_Rz, 0, 0 },
    { kHIDUsage_GD_X, kHIDUsage_GD_Y, kHIDUsage_GD_Z,  kHIDUsage_GD_Rz, 0, 0 },
    { kHIDUsage_GD_X, kHIDUsage_GD_Y, kHIDUsage_GD_Z,  kHIDUsage_GD_Rz, kHIDUsage_GD_Rx, kHIDUsage_GD_Ry },
    { kHIDUsage_GD_X, kHIDUsage_GD_Y, kHIDUsage_GD_Z,  kHIDUsage_GD_Rz, 0, 0 },
    { kHIDUsage_GD_X, kHIDUsage_GD_Y, kHIDUsage_GD_Z,  kHIDUsage_GD_Rz, 0, 0 },
};

static const uint32 joyButtonsTable[][JOY_MAX_BUTTONS] = {
    { jkA, jkB, jkX, jkY, jkLB, jkRB, jkLT, jkRT, jkStart, jkSelect, jkNone, jkUp, jkDown, jkLeft, jkRight, jkNone, jkNone },
    { jkB, jkA, jkNone, jkY, jkX, jkNone, jkLB, jkRB, jkLT, jkRT, jkSelect, jkStart, jkNone, jkL, jkR, jkNone, jkNone },
    { jkX, jkA, jkB, jkY, jkLB, jkRB, jkLT, jkRT, jkSelect, jkStart, jkL, jkR, jkNone, jkNone, jkNone, jkNone, jkNone },
    { jkX, jkA, jkB, jkY, jkLB, jkRB, jkLT, jkRT, jkSelect, jkStart, jkL, jkR, jkNone, jkNone, jkNone, jkNone, jkNone },
    { jkSelect, jkL, jkR, jkStart, jkUp, jkRight, jkDown, jkLeft, jkLT, jkRT, jkLB, jkRB, jkY, jkB, jkA, jkX, jkNone },
    { jkA, jkB, jkNone, jkX, jkY, jkNone, jkLB, jkRB, jkLT, jkRT, jkSelect, jkStart, jkNone, jkL, jkR, jkNone, jkNone },
};

JoyKey joyButtonToKey(const uint32 *btns, uint32 button) {
    if (button >= 0 || button < JOY_MAX_BUTTONS)
        return JoyKey(btns[button]);
    return jkNone;
}

void joyGetInfo(IOHIDDeviceRef device, uint32 &vendorId, uint32 &productId) {
    vendorId  = [(NSNumber*)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey)) intValue];
    productId = [(NSNumber*)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey)) intValue];
}

uint32 joyGetVendorIndex(IOHIDDeviceRef device) {
    uint32 vendorId, productId;
    joyGetInfo(device, vendorId, productId);
    
    for (int i = 0; i < JOY_MAX_VENDORS; i++) {
        if (JOY_VENDORS[i].vendorId == vendorId && (!JOY_VENDORS[i].productId || JOY_VENDORS[i].productId == productId)) {
            return i;
        }
    }
    return 0; // MS Xbox 360 as default
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
    
    uint32 page   = IOHIDElementGetUsagePage(element);
    uint32 usage  = IOHIDElementGetUsage(element);
    uint32 vendor = joyGetVendorIndex(joyDevices[joyIndex]);
    
    const uint32 *axis = joyAxisTable[vendor];
    const uint32 *btns = joyButtonsTable[vendor];
    
    switch (page) {
        case kHIDPage_GenericDesktop : {
            if (usage == axis[0])
                Input::setJoyPos(joyIndex, jkL, vec2(joyAxisValue(value), Input::joy[joyIndex].L.y));
            else if (usage == axis[1])
                Input::setJoyPos(joyIndex, jkL, vec2(Input::joy[joyIndex].L.x, joyAxisValue(value)));
            else if (usage == axis[2])
                Input::setJoyPos(joyIndex, jkR, vec2(joyAxisValue(value), Input::joy[joyIndex].R.y));
            else if (usage == axis[3])
                Input::setJoyPos(joyIndex, jkR, vec2(Input::joy[joyIndex].R.x, joyAxisValue(value)));
            else if (usage == axis[4])
                Input::setJoyPos(joyIndex, jkLT, vec2(joyAxisValue(value) * 0.5f + 0.5f, 0.0f));
            else if (usage == axis[5])
                Input::setJoyPos(joyIndex, jkRT, vec2(joyAxisValue(value) * 0.5f + 0.5f, 0.0f));
            else if (usage == kHIDUsage_GD_Hatswitch) {
                CFIndex p = IOHIDValueGetIntegerValue(value);
                Input::setJoyDown(joyIndex, jkUp,    p == 7 || p == 0 || p == 1);
                Input::setJoyDown(joyIndex, jkRight, p == 1 || p == 2 || p == 3);
                Input::setJoyDown(joyIndex, jkDown,  p == 3 || p == 4 || p == 5);
                Input::setJoyDown(joyIndex, jkLeft,  p == 5 || p == 6 || p == 7);
            }
            //if (usage != axis[0] && usage != axis[1] && usage != axis[2] && usage != axis[3])
            //    LOG("! joy: axis 0x%x (%d)\n", usage, (int)IOHIDValueGetIntegerValue(value));
            break;
        }
        case kHIDPage_Button : {
            uint32_t button = IOHIDElementGetUsage(IOHIDValueGetElement(value)) - kHIDUsage_Button_1;
            bool down  = IOHIDValueGetIntegerValue(value) != 0;
            JoyKey key = joyButtonToKey(btns, button);
            Input::setJoyDown(joyIndex, key, down);
            //LOG("! joy: button %d\n", button);
            break;
        }
        default : ;
    }
}

void joyAdd(void* context, IOReturn, void*, IOHIDDeviceRef device) {
    for (int i = 0; i < COUNT(joyDevices); i++) {
        if (joyDevices[i] == device) {
            return;
        }
    }

    for (int i = 0; i < COUNT(joyDevices); i++) {
        if (joyDevices[i] == NULL) {
            uint32 vendorId, productId;
            joyGetInfo(device, vendorId, productId);
            LOG("! joy: add index:%d vendor:0x%04X product:0x%04X\n", i, vendorId, productId);
            joyDevices[i] = device;
            break;
        }
    }
}

void joyRemove(void* context, IOReturn, void*, IOHIDDeviceRef device) {
    for (int i = 0; i < COUNT(joyDevices); i++) {
        if (joyDevices[i] == device) {
            LOG("! joy: remove index:%d\n", i);
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
    NSDictionary *matchingJoystick = @{
                                       @(kIOHIDDeviceUsagePageKey): @(kHIDPage_GenericDesktop),
                                       @(kIOHIDDeviceUsageKey): @(kHIDUsage_GD_Joystick)
                                       };
    NSArray *matchDicts = @[ matchingGamepad, matchingJoystick ];
    
    IOHIDManagerSetDeviceMatchingMultiple(hidManager, (__bridge CFArrayRef) matchDicts);
    IOHIDManagerRegisterDeviceMatchingCallback(hidManager, joyAdd, NULL);
    IOHIDManagerRegisterDeviceRemovalCallback(hidManager, joyRemove, NULL);
    IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
    IOHIDManagerRegisterInputValueCallback(hidManager, hidValueCallback, NULL);
    
    CFSetRef devices = IOHIDManagerCopyDevices(hidManager);
    if (devices) {
        CFIndex count = CFSetGetCount(devices);
        CFTypeRef devicesArray[count]; // array of IOHIDDeviceRef
        CFSetGetValues(devices, devicesArray);
        for (int i = 0; i < count; i++) {
            joyAdd(NULL, 0, NULL, (IOHIDDeviceRef)devicesArray[i]);
        }
        CFRelease(devices);
    }
}

void joyFree() {
    IOHIDManagerUnscheduleFromRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDManagerRegisterDeviceMatchingCallback(hidManager, NULL, 0);
    IOHIDManagerRegisterDeviceRemovalCallback(hidManager, NULL, 0);
    IOHIDManagerClose(hidManager, kIOHIDOptionsTypeNone);
}

NSWindow *window;

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
    if (keyCode == 36 && Input::down[ikAlt]) { // Alt + Enter
        allowFrameUpdate = NO;
        [window toggleFullScreen:nil];
        allowFrameUpdate = YES;
    }
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
    CGSize drawableSize = self.bounds.size;
    
    if (enableRetina)
    {
        NSScreen* screen = self.window.screen ?: [NSScreen mainScreen];
        drawableSize.width *= screen.backingScaleFactor;
        drawableSize.height *= screen.backingScaleFactor;
    }
    
    Core::width  = drawableSize.width;
    Core::height = drawableSize.height;
}

- (void)prepareOpenGL {
    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
}

int lastDeltaMs = 1;
BOOL allowFrameUpdate = YES;

- (void)drawRect:(NSRect)dirtyRect {
    
    if(!allowFrameUpdate) {
        return;
    }
    
    int startDrawMs = osGetTime();
    
    NSOpenGLContext *context = [self openGLContext];
    
    if (!Game::update())
        return;
    Game::render();
    
    [context flushBuffer];
    
    int endDrawMs = osGetTime();
    int deltaMs = endDrawMs - startDrawMs;
    float avgDelta = ((lastDeltaMs + deltaMs) / 2.0);
    lastDeltaMs = deltaMs;
    
    float nextDelaySec = (1000.0 / frameLimit) / 1000.0;
    nextDelaySec = nextDelaySec - (avgDelta / 1000.0);
    
    BOOL arg = YES;
    NSInvocation *inv = [NSInvocation invocationWithMethodSignature:[self methodSignatureForSelector:@selector(setNeedsDisplay:)]];
    [inv setSelector:@selector(setNeedsDisplay:)];
    [inv setTarget:self];
    [inv setArgument:&arg atIndex:2];
    [inv performSelector:@selector(invoke) withObject:self afterDelay:nextDelaySec];
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

    //get and create support path
    NSString *appName, *supportPath = nil;
    NSArray *paths = NSSearchPathForDirectoriesInDomains( NSApplicationSupportDirectory,
                                                         NSUserDomainMask, YES );
    if ( [paths count] > 0)
    {
        appName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleExecutable"];
        supportPath = [[paths objectAtIndex:0] stringByAppendingPathComponent:appName];
        
        if ( ![[NSFileManager defaultManager] fileExistsAtPath:supportPath] )
            if ( ![[NSFileManager defaultManager] createDirectoryAtPath:supportPath attributes:nil] )
                supportPath = nil;
    }
    
    cacheDir[0] = saveDir[0] = contentDir[0] = 0;
    
    NSApplication *application = [NSApplication sharedApplication];
    
    // init window
    NSRect rect = NSMakeRect(0, 0, 1280, 720);
    window = [[NSWindow alloc] initWithContentRect:rect styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask backing:NSBackingStoreBuffered defer:YES];
    window.title = @"OpenLara";
    window.acceptsMouseMovedEvents = YES;
    window.delegate = [[OpenLaraWindowDelegate alloc] init];
    
    
    // init OpenGL context
    NSOpenGLPixelFormatAttribute attribs[] = {
        NSOpenGLPFANoRecovery,
        NSOpenGLPFAMultisample,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize,     32,
        NSOpenGLPFADepthSize,     24,
        NSOpenGLPFAStencilSize,   8,
        NSOpenGLPFASampleBuffers, (NSOpenGLPixelFormatAttribute)(MSAA == 0 ? 0 : 1),
        NSOpenGLPFASamples, (NSOpenGLPixelFormatAttribute)MSAA,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
        0
    };
    NSOpenGLPixelFormat *format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
    
    OpenLaraGLView *view = [[OpenLaraGLView alloc] initWithFrame:window.contentLayoutRect pixelFormat:format];
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    window.contentView = view;
    [view.openGLContext makeCurrentContext];
    [view setWantsBestResolutionOpenGLSurface:enableRetina];
    
    // get path to game content
    NSBundle *bundle   = [NSBundle mainBundle];
    NSURL *resourceURL = bundle.resourceURL;
    [resourceURL getFileSystemRepresentation:contentDir maxLength:sizeof(contentDir)];
    strcat(contentDir, "/");
    
    [supportPath getCString:saveDir maxLength:sizeof(saveDir) encoding:NSUTF8StringEncoding];
    strcat(saveDir, "/");
    [supportPath getCString:cacheDir maxLength:sizeof(cacheDir) encoding:NSUTF8StringEncoding];
    strcat(cacheDir, "/");
    
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
