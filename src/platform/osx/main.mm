#include <Cocoa/Cocoa.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

#include "game.h"

// sound
#define SND_SIZE 2352

static AudioQueueRef audioQueue;

void soundFill(void* inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer) {
    void* frames = inBuffer->mAudioData;
    UInt32 count = inBuffer->mAudioDataBytesCapacity / 4;
    Sound::fill((Sound::Frame*)frames, count);
    inBuffer->mAudioDataByteSize = count * 4;
    AudioQueueEnqueueBuffer(audioQueue, inBuffer, 0, NULL);
}

void soundInit() {
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
    
    AudioQueueNewOutput(&deviceFormat, soundFill, NULL, NULL, NULL, 0, &audioQueue);
    
    for (int i = 0; i < 2; i++) {
        AudioQueueBufferRef mBuffer;
        AudioQueueAllocateBuffer(audioQueue, SND_SIZE, &mBuffer);
        soundFill(NULL, audioQueue, mBuffer);
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
                LOG("%d\n", code);
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

bool osJoyReady(int index) {
    return false;
}

void osJoyVibrate(int index, float L, float R) {
    // TODO
}

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
    NSWindow *mainWindow = [[NSWindow alloc] initWithContentRect:rect styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask backing:NSBackingStoreBuffered defer:YES];
    mainWindow.title = @"OpenLara";
    mainWindow.acceptsMouseMovedEvents = YES;
    mainWindow.delegate = [[OpenLaraWindowDelegate alloc] init];
    
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

    OpenLaraGLView *view = [[OpenLaraGLView alloc] initWithFrame:mainWindow.contentLayoutRect pixelFormat:format];
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    mainWindow.contentView = view;
    [view.openGLContext makeCurrentContext];
    
    // get path to game content
    NSBundle *bundle   = [NSBundle mainBundle];
    NSURL *resourceURL = bundle.resourceURL;
    [resourceURL getFileSystemRepresentation:contentDir maxLength:sizeof(contentDir)];
    strcat(contentDir, "/");

    // show window
    [mainWindow center];
    [mainWindow makeKeyAndOrderFront:nil];

    soundInit();
    Game::init();

    if (!Core::isQuit) {
        [application run];
    }
    
    Game::deinit();
    // TODO: sndFree

    return 0;
}
