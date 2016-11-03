#include "game.h"

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

OSStatus eventHandler(EventHandlerCallRef handler, EventRef event, void* userData) {
    OSType eventClass   = GetEventClass(event);
    UInt32 eventKind    = GetEventKind(event);

    switch (eventClass) {
        case kEventClassWindow :
            switch (eventKind) {
                case kEventWindowClosed :
                    isQuit = true;
                    break;
                case kEventWindowBoundsChanged : {
                    aglUpdateContext(context);
                    Rect rect;
                    GetWindowPortBounds(window, &rect);
                    Core::width  = rect.right - rect.left;
                    Core::height = rect.bottom - rect.top;
                    break;
                }
            }
            break;

        case kEventClassMouse : {
            EventMouseButton mouseButton;
            CGPoint mousePos;
            Rect wndRect;

            GetEventParameter(event, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(mousePos), NULL, &mousePos);
            GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(mouseButton), nil, &mouseButton);

            GetWindowBounds(window, kWindowContentRgn, &wndRect);
            mousePos.x -= wndRect.left;
            mousePos.y -= wndRect.top;
            vec2 pos(mousePos.x, mousePos.y);

            switch (eventKind) {
                case kEventMouseDown    :
                case kEventMouseUp      : {
                    InputKey key = mouseToInputKey(mouseButton);
                    Input::setPos(key, pos);
                    Input::setDown(key, eventKind == kEventMouseDown);
                    break;
                }
                case kEventMouseDragged :
                    Input::setPos(ikMouseL, pos);
                    break;
            }
            break;
        }

        case kEventClassKeyboard : {
            switch (eventKind) {
                case kEventRawKeyDown :
                case kEventRawKeyUp   : {
                    uint32 keyCode;
                    if (GetEventParameter(event, kEventParamKeyCode, typeUInt32, NULL, sizeof(keyCode), NULL, &keyCode) == noErr)
                        Input::setDown(keyToInputKey(keyCode), eventKind != kEventRawKeyUp);
                    break;
                }
                case kEventRawKeyModifiersChanged : {
                    uint32 modifiers;
                    if (GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers) == noErr) {
                        Input::setDown(ikShift, modifiers & shiftKey);
                        Input::setDown(ikCtrl,  modifiers & controlKey);
                        Input::setDown(ikAlt,   modifiers & optionKey);
                    }
                    break;
                }
            }
            break;
        }
    }

    return CallNextEventHandler(handler, event);
}

int getTime() {
    UInt64 t;
    Microseconds((UnsignedWide*)&t);
    return int(t / 1000);
}

char *contentPath;

int main() {
// init window
    Rect rect = {0, 0, 720, 1280};
    CreateNewWindow(kDocumentWindowClass, kWindowCloseBoxAttribute | kWindowCollapseBoxAttribute | kWindowFullZoomAttribute | kWindowResizableAttribute | kWindowStandardHandlerAttribute, &rect, &window);
    SetWTitle(window, "\pOpenLara");

    // init OpenGL context
    GLint attribs[] = {
        AGL_RGBA,
        AGL_DOUBLEBUFFER,
        AGL_BUFFER_SIZE,    32,
        AGL_DEPTH_SIZE,     24,
        AGL_STENCIL_SIZE,   8,
        AGL_NONE
    };
    AGLPixelFormat format = aglChoosePixelFormat(NULL, 0, (GLint*)&attribs);
    context = aglCreateContext(format, NULL);
    aglDestroyPixelFormat(format);

    aglSetDrawable(context, GetWindowPort(window));
    aglSetCurrentContext(context);

    // get path to game content
    CFBundleRef bundle  = CFBundleGetMainBundle();
    CFURLRef bundleURL  = CFBundleCopyBundleURL(bundle);
    CFStringRef pathStr = CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle);
    contentPath = new char[1024];
    CFStringGetFileSystemRepresentation(pathStr, contentPath, 1024);
    strcat(contentPath, "/Contents/Resources/");

    soundInit();
    Game::init();

    // show window
    const int events[][2] = {
        { kEventClassWindow,    kEventWindowClosed           },
        { kEventClassWindow,    kEventWindowBoundsChanged    },
        { kEventClassKeyboard,  kEventRawKeyDown             },
        { kEventClassKeyboard,  kEventRawKeyUp               },
        { kEventClassKeyboard,  kEventRawKeyModifiersChanged },
        { kEventClassMouse,     kEventMouseDown              },
        { kEventClassMouse,     kEventMouseUp                },
        { kEventClassMouse,     kEventMouseDragged           },
    };

    InstallEventHandler(GetApplicationEventTarget(), (EventHandlerUPP)eventHandler, sizeof(events) / sizeof(events[0]), (EventTypeSpec*)&events, NULL, NULL);
    SelectWindow(window);
    ShowWindow(window);

    int lastTime = getTime(), fpsTime = lastTime + 1000, fps = 0;

    EventRecord event;
    while (!isQuit)
        if (!GetNextEvent(0xffff, &event)) {
            int time = getTime();
            if (time <= lastTime)
                continue;

            float delta = (time - lastTime) * 0.001f;
            while (delta > EPS) {
                Core::deltaTime = min(delta, 1.0f / 30.0f);
                Game::update();
                delta -= Core::deltaTime;
            }
            lastTime = time;

            Core::stats.dips = 0;
            Core::stats.tris = 0;
            Game::render();
            aglSwapBuffers(context);

            if (fpsTime < getTime()) {
                LOG("FPS: %d DIP: %d TRI: %d\n", fps, Core::stats.dips, Core::stats.tris);
                fps = 0;
                fpsTime = getTime() + 1000;
            } else
                fps++;
        }

    Game::free();
    delete[] contentPath;
	// TODO: sndFree

    aglSetCurrentContext(NULL);
    ReleaseWindow(window);

    return 0;
}
