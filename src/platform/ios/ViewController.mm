#import "ViewController.h"
#include <AudioToolbox/AudioQueue.h>
#include <AudioToolbox/AudioFile.h>
#include <AudioToolbox/AudioServices.h>
#include <AudioToolbox/AudioConverter.h>
#include <AudioToolbox/AudioToolbox.h>
#include "game.h"

@interface ViewController ()
@property (strong, nonatomic) EAGLContext *context;
@end

@implementation ViewController

@synthesize context = _context;

#include <mach/mach_time.h>

char Stream::cacheDir[255];
char Stream::contentDir[255];

int lastTime;

int getTime() {
    const int64_t kOneMillion = 1000 * 1000;
    static mach_timebase_info_data_t info;

    if (info.denom == 0)
        mach_timebase_info(&info);

    return (int)((mach_absolute_time() * info.numer) / (kOneMillion * info.denom));
}

#define SND_BUF_SIZE 8192

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
    deviceFormat.mSampleRate 		= 44100;
    deviceFormat.mFormatID 			= kAudioFormatLinearPCM;
    deviceFormat.mFormatFlags 		= kLinearPCMFormatFlagIsSignedInteger;
    deviceFormat.mBytesPerPacket	= 4;
    deviceFormat.mFramesPerPacket	= 1;
    deviceFormat.mBytesPerFrame		= 4;
    deviceFormat.mChannelsPerFrame	= 2;
    deviceFormat.mBitsPerChannel	= 16;
    deviceFormat.mReserved			= 0;

    AudioQueueNewOutput(&deviceFormat, soundFill, NULL, NULL, kCFRunLoopCommonModes, 0, &audioQueue);

    for (int i = 0; i < 2; i++) {
        AudioQueueBufferRef mBuffer;
        AudioQueueAllocateBuffer(audioQueue, SND_BUF_SIZE, &mBuffer);
        soundFill(NULL, audioQueue, mBuffer);
    }
    AudioQueueStart(audioQueue, NULL);
}

- (id)init
{
    self = [super init];
	[[NSMutableDictionary alloc] init];
    m_touchList = [[NSMutableArray alloc] init];
    for (int i=0; i<10; i++)
        [m_touchList addObject:[NSNull null]];
    return self;
}

- (void)dealloc
{
    AudioQueueDispose(audioQueue, true);
    [m_touchList release];
    [super dealloc];
}

- (void)viewDidLoad {
	[super viewDidLoad];

	self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
		NSLog(@"! core: failed to create ES context");
        return;
    }

    self.preferredFramesPerSecond = 60;

    GLKView *view = (GLKView *)self.view;
    view.multipleTouchEnabled = YES;
    view.context = self.context;
	view.drawableDepthFormat = GLKViewDrawableDepthFormat16;
	view.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;
	[EAGLContext setCurrentContext:self.context];
	

    Stream::contentDir[0] = Stream::cacheDir[0] = 0;

    NSString *path = [[NSBundle mainBundle] resourcePath];
    strcat(Stream::contentDir, [path UTF8String]);
    strcat(Stream::contentDir, "/");

    Stream *lvl = new Stream("LEVEL2.PSX");
    Stream *snd = new Stream("05.ogg");

    Game::init(lvl, snd);

    soundInit();
    Input::reset();

    lastTime = getTime() - 1;
}

- (void)viewDidUnload {	
	[super viewDidUnload];
    Game::free();
	if ([EAGLContext currentContext] == self.context)
		[EAGLContext setCurrentContext:nil];
	self.context = nil;
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect {
    [((GLKView *) self.view) bindDrawable];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&Core::defaultFBO);

	float scale  = [[UIScreen mainScreen] scale];
    Core::width  = self.view.bounds.size.width * scale;
    Core::height = self.view.bounds.size.height * scale;

    int time = getTime();
    if (time == lastTime)
        return;
    Game::update((time - lastTime) * 0.001f);
    lastTime = time;

	Game::render();
}

- (void) doTouch:(UIEvent*)event {
    float scale = [[UIScreen mainScreen] scale];

    NSSet* touchSet = [event allTouches];
    for (UITouch *touch in touchSet) {
        if (touch.phase == UITouchPhaseStationary)
            continue;

        CGPoint   pos = [touch locationInView:self.view];
        NSUInteger id = int(touch);

        InputKey key = Input::getTouch(id);
        if (key == ikNone) return;
        Input::setPos(key, vec2(pos.x, pos.y) * scale);

        if (touch.phase != UITouchPhaseMoved) {
            Input::setDown(key, touch.phase == UITouchPhaseBegan);
        }
    }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	[self doTouch:event];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	[self doTouch:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    [self doTouch:event];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    [self doTouch:event];
}

@end
