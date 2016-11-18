#include "game.h"

#include <Cocoa/Cocoa.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

bool isQuit = false;
WindowRef window;
AGLContext context;

#define SND_SIZE 8192

static AudioQueueRef audioQueue;

void soundFill(void* inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer) {
    void* frames = inBuffer->mAudioData;
    UInt32 count = inBuffer->mAudioDataBytesCapacity / 4;
    Sound::fill((Sound::Frame*)frames, count);
    inBuffer->mAudioDataByteSize = count * 4;
    AudioQueueEnqueueBuffer(audioQueue, inBuffer, 0, NULL);
	// TODO: mutex
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
        0x7B, 0x7C, 0x7E, 0x7D, 0x31, 0x24, 0x35, 0x38, 0x3B, 0x3A,
        0x1D, 0x12, 0x13, 0x14, 0x15, 0x17, 0x16, 0x1A, 0x1C, 0x19,                   // 0..9
        0x00, 0x0B, 0x08, 0x02, 0x0E, 0x03, 0x05, 0x04, 0x22, 0x26, 0x28, 0x25, 0x2E, // A..M
        0x2D, 0x1F, 0x23, 0x0C, 0x0F, 0x01, 0x11, 0x20, 0x09, 0x0D, 0x07, 0x10, 0x06, // N..Z
    };

    for (int i = 0; i < sizeof(codes) / sizeof(codes[0]); i++)
        if (codes[i] == code)
            return (InputKey)(ikLeft + i);
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

int lastTime;
int fpsTime;
int fps;
CVDisplayLinkRef displayLink;

int getTime() {
    static mach_timebase_info_data_t timebaseInfo;
    if (timebaseInfo.denom == 0) {
        mach_timebase_info(&timebaseInfo);
    }
    
    uint64_t absolute = mach_absolute_time();
    uint64_t milliseconds = absolute * timebaseInfo.numer / (timebaseInfo.denom * 1000000ULL);
    return int(milliseconds);
}

/*
 * Specific OpenGLView. This subclass is necessary primarily to handle input.
 * Capturing and dispatching events manually on OS X is definitely not worth it.
 */
@interface OpenLaraGLView : NSOpenGLView

@end

@implementation OpenLaraGLView

- (InputKey)inputKeyForMouseEvent:(NSEvent *)theEvent {
    switch (theEvent.buttonNumber) {
        case 0: return ikMouseL;
        case 1: return ikMouseR;
        case 2: return ikMouseM;
        default: return ikNone;
    }
}

- (vec2)inputPositionForMouseEvent:(NSEvent *)theEvent {
    NSPoint inWindow = theEvent.locationInWindow;
    NSPoint inView = [self convertPoint:inWindow fromView:nil];
    // TODO Do we need to flip y, due to OS X having the origin at the bottom
    // left as opposed to top left in every single other system? The original
    // code didn't so I won't either for now.
    return vec2(inView.x, inView.y);
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
    Input::setDown(ikShift, modifiers & NSShiftKeyMask);
    Input::setDown(ikCtrl,  modifiers & NSControlKeyMask);
    Input::setDown(ikAlt,   modifiers & NSAlternateKeyMask);
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)reshape {
    NSRect bounds = self.bounds;
    Core::width  = bounds.size.width;
    Core::height = bounds.size.height;
}

@end

/*
 * Delegate to deal with things that happen at the window level
 */
@interface OpenLaraWindowDelegate : NSObject<NSWindowDelegate>
@end

@implementation OpenLaraWindowDelegate

- (void)windowWillClose:(NSNotification *)notification {
    [[NSApplication sharedApplication] terminate:self];
}

- (void)windowDidMiniaturize:(NSNotification *)notification {
    // Pause game
    CVDisplayLinkStop(displayLink);
    Input::reset();
}

- (void)windowDidDeminiaturize:(NSNotification *)notification {
    // End paused game.
    lastTime = getTime();
    CVDisplayLinkStart(displayLink);
}

@end

char *contentPath;

/*
 * Callback for the CVDisplayLink, an OS X mechanism to get precise timing for
 * multi-media applications. This runs the whole game loop, for simplicitly's
 * sake. This is not really the idea of the displayLinkCallback, which should
 * more or less just swap the OpenGL buffer here and at least have the update
 * running in a different thread entirely. But it works.
 */
CVReturn displayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *now, const CVTimeStamp *outputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *context) {
    OpenLaraGLView *view = (OpenLaraGLView *) context;
    [view.openGLContext makeCurrentContext];
    
    // TODO: This should probably get the time from the outputTime parameter
    int time = getTime();
    if (time <= lastTime)
        return kCVReturnUnsupported;
    
    // TODO: This should probably run the update in a separate thread
    // and only do rendering here
    float delta = (time - lastTime) * 0.001f;
    while (delta > EPS) {
        Core::deltaTime = min(delta, 1.0f / 30.0f);
        Game::update();
        delta -= Core::deltaTime;
    }
    lastTime = time;
    
    // TODO: Rendering should probably happen a bit in advance with only the
    // flushBuffer here
    Core::stats.dips = 0;
    Core::stats.tris = 0;
    Game::render();
    [view.openGLContext flushBuffer];
    
    if (fpsTime < getTime()) {
        LOG("FPS: %d DIP: %d TRI: %d\n", fps, Core::stats.dips, Core::stats.tris);
        fps = 0;
        fpsTime = getTime() + 1000;
    } else
        fps++;

    return kCVReturnSuccess;
}

int main() {
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
        NSOpenGLPFAColorSize, 32,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAStencilSize, 8,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
        0
    };
    NSOpenGLPixelFormat *format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
    
    OpenLaraGLView *view = [[OpenLaraGLView alloc] initWithFrame:mainWindow.contentLayoutRect pixelFormat:format];
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    mainWindow.contentView = view;
    [view.openGLContext makeCurrentContext];

    // get path to game content
    NSBundle *bundle  = [NSBundle mainBundle];
    NSURL *bundleURL  = bundle.bundleURL;
    contentPath = new char[1024];
    [bundleURL getFileSystemRepresentation:contentPath maxLength:1024];
    strcat(contentPath, "/Contents/Resources/");

    soundInit();
    Game::init();
    
    // show window
    [mainWindow makeKeyAndOrderFront:nil];
    
    // Set up DisplayLink. This will call our callback in time with display
    // refresh rate.
    CVReturn cvreturn = CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    if (cvreturn != kCVReturnSuccess) {
        NSLog(@"Could not create Display Link: %d", (int) cvreturn);
    }
    cvreturn = CVDisplayLinkSetOutputCallback(displayLink, displayLinkCallback, view);
    if (cvreturn != kCVReturnSuccess) {
        NSLog(@"Could not create set callback for display link: %d", (int) cvreturn);
    }
    
    lastTime = getTime();
    fpsTime = lastTime + 1000;
    cvreturn = CVDisplayLinkStart(displayLink);
    if (cvreturn != kCVReturnSuccess) {
        NSLog(@"Could not start display link: %d", (int) cvreturn);
    }
    
    // Start application main loop
    [application run];
    return 0;
}
