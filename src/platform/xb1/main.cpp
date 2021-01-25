#include <ppltasks.h>
#include <xaudio2.h>
#include <wrl/client.h>
#include <dxgi1_4.h>

#include "game.h"

using namespace Concurrency;
using namespace Windows;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics;
using namespace Windows::Gaming::Input;
using namespace Windows::Storage;
using namespace Windows::Globalization;
using namespace Windows::System::UserProfile;
using namespace Microsoft::WRL;


ID3D11Device        *device;
ID3D11DeviceContext *deviceContext;
IDXGISwapChain1     *swapChain;


// multi-threading
void* osMutexInit()
{
    CRITICAL_SECTION *CS = new CRITICAL_SECTION();
    InitializeCriticalSection(CS);
    return CS;
}

void osMutexFree(void *obj)
{
    DeleteCriticalSection((CRITICAL_SECTION*)obj);
    delete (CRITICAL_SECTION*)obj;
}

void osMutexLock(void *obj)
{
    EnterCriticalSection((CRITICAL_SECTION*)obj);
}

void osMutexUnlock(void *obj)
{
    LeaveCriticalSection((CRITICAL_SECTION*)obj);
}


// timing
LARGE_INTEGER timerFreq;
LARGE_INTEGER timerStart;

int osGetTimeMS()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return int32((time.QuadPart - timerStart.QuadPart) * 1000L / timerFreq.QuadPart);
}


// input
struct JoyDevice {
    Gamepad^          gamepad;
    int               time;
    float             vL, vR;
    float             oL, oR;
} joyDevice[INPUT_JOY_COUNT];

Core::Mutex joyLock;

#define JOY_MIN_UPDATE_FX_TIME  50
#define JOY_TRIGGER_THRESHOLD   0.1f

void osJoyVibrate(int index, float L, float R)
{
    joyDevice[index].vL = L;
    joyDevice[index].vR = R;
}

float joyTrigger(double value)
{
    return clamp(((float)value - JOY_TRIGGER_THRESHOLD) / (1.0f - JOY_TRIGGER_THRESHOLD), 0.0f, 1.0f);
}

void joyUpdate()
{
    OS_LOCK(joyLock);

    #define JOY_BUTTON(index, btn, mask) Input::setJoyDown(index, btn, (state.Buttons & mask) == mask);

    for (int i = 0; i < INPUT_JOY_COUNT; i++)
    {
        JoyDevice &joy = joyDevice[i];

        if (!joy.gamepad) continue;

        GamepadReading& state = joy.gamepad->GetCurrentReading();

        JOY_BUTTON(i, jkA,      GamepadButtons::A);
        JOY_BUTTON(i, jkB,      GamepadButtons::B);
        JOY_BUTTON(i, jkX,      GamepadButtons::X);
        JOY_BUTTON(i, jkY,      GamepadButtons::Y);
        JOY_BUTTON(i, jkLeft,   GamepadButtons::DPadLeft);
        JOY_BUTTON(i, jkRight,  GamepadButtons::DPadRight);
        JOY_BUTTON(i, jkUp,     GamepadButtons::DPadUp);
        JOY_BUTTON(i, jkDown,   GamepadButtons::DPadDown);
        JOY_BUTTON(i, jkSelect, GamepadButtons::View);
        JOY_BUTTON(i, jkStart,  GamepadButtons::Menu);
        JOY_BUTTON(i, jkL,      GamepadButtons::LeftThumbstick);
        JOY_BUTTON(i, jkR,      GamepadButtons::RightThumbstick);
        JOY_BUTTON(i, jkLB,     GamepadButtons::LeftShoulder);
        JOY_BUTTON(i, jkRB,     GamepadButtons::RightShoulder);

        Input::setJoyPos(i, jkL, vec2(float(state.LeftThumbstickX), -float(state.LeftThumbstickY)));
        Input::setJoyPos(i, jkR, vec2(float(state.RightThumbstickX), -float(state.RightThumbstickY)));
        Input::setJoyPos(i, jkLT, vec2(joyTrigger(state.LeftTrigger), 0.0f));
        Input::setJoyPos(i, jkRT, vec2(joyTrigger(state.RightTrigger), 0.0f));

        if ((joy.vL != joy.oL || joy.vR != joy.oR) && osGetTimeMS() >= joy.time) {
            GamepadVibration vibration;
            vibration.LeftMotor = joy.vL;
            vibration.RightMotor = joy.vR;
            joy.gamepad->Vibration = vibration;
            joy.oL = joy.vL;
            joy.oR = joy.vR;
            joy.time = osGetTimeMS() + JOY_MIN_UPDATE_FX_TIME;
        }
    }

    #undef JOY_BUTTON
}

void joyAdd(Platform::Object^, Gamepad^ args)
{
    OS_LOCK(joyLock);
    for (int i = 0; i < INPUT_JOY_COUNT; i++)
    {
        JoyDevice &joy = joyDevice[i];
        if (joy.gamepad) continue;
        joy.gamepad = args;
        return;
    }
}

void joyRemove(Platform::Object^, Gamepad^ args)
{
    OS_LOCK(joyLock);
    for (int i = 0; i < INPUT_JOY_COUNT; i++)
    {
        JoyDevice &joy = joyDevice[i];
        if (!joy.gamepad) continue;
        if (joy.gamepad == args) {
            joy = {};
            return;
        }
    }
}

void joyInit()
{
    Gamepad::GamepadAdded += ref new EventHandler<Gamepad^>(&joyAdd);
    Gamepad::GamepadRemoved += ref new EventHandler<Gamepad^>(&joyRemove);
}


// sound
#define SND_SIZE        4704*sizeof(int16)
#define SND_MAX_BUFFERS 2

struct AudioContext : public IXAudio2VoiceCallback
{
    Microsoft::WRL::ComPtr<IXAudio2> pXAudio2;
    HANDLE event;

    virtual void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
    virtual void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
    virtual void STDMETHODCALLTYPE OnStreamEnd() override {}
    virtual void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
    virtual void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}
    virtual void STDMETHODCALLTYPE OnBufferStart(void*) override {}

    virtual void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext)
    {
        SetEvent(event);
    }

    AudioContext()
    {
        event = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
    }

    virtual ~AudioContext()
    {
        CloseHandle(event);
    }

    void start()
    {
        CreateThread(NULL, 0, fill, this, 0, NULL);    
    }

    void stop() {
        SetEvent(event);
    }

    void suspend() {
        pXAudio2->StopEngine();
    }

    void resume() {
        pXAudio2->StartEngine();
    }

    static DWORD WINAPI fill(LPVOID lpParam)
    {
        AudioContext* context = (AudioContext*)lpParam;

        IXAudio2MasteringVoice*          masteringVoice;
        IXAudio2SourceVoice*             sourceVoice;
        uint32                           bufferIndex = 0;

        if (FAILED(XAudio2Create(context->pXAudio2.GetAddressOf(), 0))) {
            return 0;
        }

        if (FAILED(context->pXAudio2->CreateMasteringVoice(&masteringVoice))) {
            return 0;
        }

        uint8* data = new uint8[SND_SIZE * SND_MAX_BUFFERS];

        WAVEFORMATEX waveFmt = { WAVE_FORMAT_PCM, 2, 44100, 44100 * 4, 4, 16, sizeof(waveFmt) };

        if (FAILED(context->pXAudio2->CreateSourceVoice(&sourceVoice, &waveFmt, 0, XAUDIO2_DEFAULT_FREQ_RATIO, context))) {
            return 0;
        }

        if (FAILED(sourceVoice->Start(0))) {
            return 0;
        }

        while (!Core::isQuit)
        {
            XAUDIO2_VOICE_STATE state;
            sourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

            if (state.BuffersQueued < SND_MAX_BUFFERS) {
                XAUDIO2_BUFFER buffer = {};
                buffer.AudioBytes = SND_SIZE;
                buffer.pAudioData = data + SND_SIZE * bufferIndex;

                Sound::fill((Sound::Frame*)buffer.pAudioData, buffer.AudioBytes / 4);

                bufferIndex = (bufferIndex + 1) % SND_MAX_BUFFERS;

                sourceVoice->SubmitSourceBuffer(&buffer);
            } else {
                WaitForSingleObject(context->event, INFINITE);
            }
        }

        delete[] data;

        return 0;
    }
};


struct RenderContext
{
    void init(CoreWindow^ window)
    {
        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };

        HRESULT ret;
        ret = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &device, NULL, &deviceContext);
        ASSERT(ret == S_OK);

        ComPtr<IDXGIDevice3> dxgiDevice;
        ComPtr<ID3D11Device>(device).As(&dxgiDevice);

        ComPtr<IDXGIAdapter> dxgiAdapter;
        dxgiDevice->GetAdapter(&dxgiAdapter);

        ComPtr<IDXGIFactory4> dxgiFactory;
        dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width                = Core::width;
        desc.Height               = Core::height;
        desc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferCount          = 2;
        desc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SampleDesc.Count     = 1;
        desc.SampleDesc.Quality   = 0;
        desc.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.Scaling              = DXGI_SCALING_NONE;
        desc.AlphaMode            = DXGI_ALPHA_MODE_IGNORE;

        ret = dxgiFactory->CreateSwapChainForCoreWindow(device, reinterpret_cast<IUnknown*>(window), &desc, NULL, &swapChain);
        ASSERT(ret == S_OK);

        dxgiDevice->SetMaximumFrameLatency(1);
    }

    void present()
    {
        swapChain->Present(Core::settings.detail.vsync ? 1 : 0, 0);
    }
};


ref class View sealed : public ApplicationModel::Core::IFrameworkView
{
private:
    AudioContext audioContext;
    RenderContext renderContext;

    bool m_windowVisible;

public:
    View() : m_windowVisible(true) {};

    virtual void Initialize(ApplicationModel::Core::CoreApplicationView^ applicationView)
    {
        applicationView->Activated += ref new TypedEventHandler<ApplicationModel::Core::CoreApplicationView^, IActivatedEventArgs^>(this, &View::OnActivated);
        ApplicationModel::Core::CoreApplication::Suspending += ref new EventHandler<ApplicationModel::SuspendingEventArgs^>(this, &View::OnSuspending);
        ApplicationModel::Core::CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &View::OnResuming);
    }

    virtual void SetWindow(CoreWindow^ window)
    {
        window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &View::OnVisibilityChanged);
        window->Closed +=  ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &View::OnWindowClosed);
        SystemNavigationManager::GetForCurrentView()->BackRequested += ref new EventHandler<BackRequestedEventArgs^>(this, &View::OnBackRequested);

        Core::width  = 1920;
        Core::height = 1080;

        renderContext.init(window);
    }

    int checkLanguage()
    {
        Platform::String^ language = GlobalizationPreferences::Languages->GetAt(0);
        const wchar_t* id = language->Begin();

        #define CHECK(str) (wcsstr(id, L##str"-") != 0)

        int str = STR_LANG_EN;

        if (CHECK("fr")) {
            str = STR_LANG_FR;
        } else if (CHECK("de")) {
            str = STR_LANG_DE;
        } else if (CHECK("es")) {
            str = STR_LANG_ES;
        } else if (CHECK("it")) {
            str = STR_LANG_IT;
        } else if (CHECK("pl")) {
            str = STR_LANG_PL;
        } else if (CHECK("pt")) {
            str = STR_LANG_PT;
        } else if (CHECK("ru") || CHECK("be") || CHECK("uk")) {
            str = STR_LANG_RU;
        } else if (CHECK("ja")) {
            str = STR_LANG_JA;
        } else if (CHECK("gr")) {
            str = STR_LANG_GR;
        } else if (CHECK("fi")) {
            str = STR_LANG_FI;
        } else if (CHECK("cs")) {
            str = STR_LANG_CZ;
        } else if (CHECK("zh")) {
            str = STR_LANG_CN;
        } else if (CHECK("hu")) {
            str = STR_LANG_HU;
        } else if (CHECK("sv")) {
            str = STR_LANG_SV;
        }

        return str - STR_LANG_EN;
    }

    virtual void Load(Platform::String^ entryPoint)
    {
        //
    }

    virtual void Run()
    {
        contentDir[0] = saveDir[0] = cacheDir[0] = 0;

        StorageFolder^ localFolder = ApplicationData::Current->LocalFolder;
        wcstombs(contentDir, localFolder->Path->Data(), sizeof(contentDir));
        strcat(contentDir, "\\");
        strcpy(saveDir, contentDir);
        strcpy(cacheDir, contentDir);

        Stream::addPack("content.zip");

        GAPI::defRTV = NULL;
        GAPI::defDSV = NULL;

        QueryPerformanceFrequency(&timerFreq);
        QueryPerformanceCounter(&timerStart);

        Sound::channelsCount = 0;

        joyInit();
        audioContext.start();

        Core::defLang = checkLanguage();

        Game::init((const char*)NULL);

        while (!Core::isQuit)
        {
            if (m_windowVisible)
            {
                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

                joyUpdate();

                if (Game::update()) {
                    Game::render();
                    renderContext.present();
                }
            } else {
                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
            }
        }

        audioContext.stop();
        Game::deinit();
    }

    virtual void Uninitialize()
    {
        // TODO save state?
    }

protected:
    void OnActivated(ApplicationModel::Core::CoreApplicationView^ applicationView, ApplicationModel::Activation::IActivatedEventArgs^ args)
    {
        CoreWindow::GetForCurrentThread()->Activate();
    }

    void OnSuspending(Platform::Object^ sender, ApplicationModel::SuspendingEventArgs^ args)
    {
        audioContext.suspend();

        ApplicationModel::SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

        create_task([this, deferral]()
        {
            // TODO save state?
            deferral->Complete();
        });
    }

    void OnResuming(Platform::Object^ sender, Platform::Object^ args)
    {
        audioContext.resume();
        // TODO load state?
    }

    void OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
    {
        m_windowVisible = args->Visible;
    }

    void OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
    {
        ::Core::quit();
    }

    void OnBackRequested(Platform::Object^, BackRequestedEventArgs^ args)
    {
        args->Handled = true;
    }
};


ref class ViewSource sealed : ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual ApplicationModel::Core::IFrameworkView^ CreateView()
    {
        return ref new View();
    }
};


[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^) {
    auto viewSource = ref new ViewSource();
    ApplicationModel::Core::CoreApplication::Run(viewSource);
    return 0;
}