#include <wrl.h>
#include <wrl/client.h>
#include <d3d11_1.h>
#include <xaudio2.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <ppl.h>
#include <ppltasks.h>

#include "game.h"

using namespace Windows;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage;
using namespace Concurrency;

// multi-threading
void* osMutexInit()
{
    CRITICAL_SECTION *CS = new CRITICAL_SECTION();
    InitializeCriticalSectionEx(CS, 0, 0);
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
int osStartTime = 0;

int osGetTimeMS()
{
    LARGE_INTEGER Freq, Count;
    QueryPerformanceFrequency(&Freq);
    QueryPerformanceCounter(&Count);
    return int(Count.QuadPart * 1000L / Freq.QuadPart);
}


// input
bool osJoyReady(int index)
{
    return false;
}

void osJoyVibrate(int index, float L, float R)
{
    //
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
        auto workItem = ref new Windows::System::Threading::WorkItemHandler([this](IAsyncAction^ workItem)
        {
            AudioContext::fill(this);
        });

        Windows::System::Threading::ThreadPool::RunAsync(workItem);
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
            }
            else {
                WaitForSingleObjectEx(context->event, INFINITE, FALSE);
            }
        }

        delete[] data;

        return 0;
    }
};


// system
Microsoft::WRL::ComPtr<ID3D11Device1>        osDevice;
Microsoft::WRL::ComPtr<ID3D11DeviceContext1> osContext;
Microsoft::WRL::ComPtr<IDXGISwapChain1>      osSwapChain;

float ConvertDipsToPixels(float dips)
{
    static const float dipsPerInch = 96.0f;
    return floor(dips * DisplayProperties::LogicalDpi / dipsPerInch + 0.5f);
}

ref class View sealed : public IFrameworkView
{
private:
    Platform::Agile<Windows::UI::Core::CoreWindow> m_window;
    AudioContext audioContext;
    bool m_windowVisible;

public:
    View() : m_windowVisible(true) {}

    void InitD3D11(Windows::UI::Core::CoreWindow^ window)
    {
        m_window = window;

        CreateDeviceResources();
        CreateWindowSizeDependentResources();
    }

    void HandleDeviceLost()
    {
        ReleaseResourcesForSuspending();
        CreateDeviceResources();
        CreateWindowSizeDependentResources();
    }

    void CreateDeviceResources()
    {
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        //creationFlags |= D3D11_CREATE_DEVICE_DEBUG;

        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_9_3
        };

        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;

        D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            creationFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &device,
            nullptr,
            &context);

        device.As(&osDevice);
        context.As(&osContext);
    }

    void CreateWindowSizeDependentResources()
    {
        Core::width = int32(ConvertDipsToPixels(m_window->Bounds.Width));
        Core::height = int32(ConvertDipsToPixels(m_window->Bounds.Height));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width = Core::width;
        swapChainDesc.Height = Core::height;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 1;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Flags = 0;

        ComPtr<IDXGIDevice1> dxgiDevice;
        osDevice.As(&dxgiDevice);

        ComPtr<IDXGIAdapter> dxgiAdapter;
        dxgiDevice->GetAdapter(&dxgiAdapter);

        ComPtr<IDXGIFactory2> dxgiFactory;
        dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &dxgiFactory);

        Windows::UI::Core::CoreWindow^ window = m_window.Get();
        dxgiFactory->CreateSwapChainForCoreWindow(
            osDevice.Get(),
            reinterpret_cast<IUnknown*>(window),
            &swapChainDesc,
            nullptr,
            &osSwapChain);

        dxgiDevice->SetMaximumFrameLatency(1);
    }

    void ReleaseResourcesForSuspending()
    {
        GAPI::resetDevice();
        osSwapChain = nullptr;
    }

    void Present()
    {
        HRESULT hr = osSwapChain->Present(1, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED)
        {
            HandleDeviceLost();
        }
    }

    virtual void Initialize(CoreApplicationView^ applicationView)
    {
        applicationView->Activated +=
            ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &View::OnActivated);

        CoreApplication::Suspending +=
            ref new EventHandler<Windows::ApplicationModel::SuspendingEventArgs^>(this, &View::OnSuspending);

        CoreApplication::Resuming +=
            ref new EventHandler<Platform::Object^>(this, &View::OnResuming);
    }

    virtual void SetWindow(CoreWindow^ window)
    {
        DisplayProperties::AutoRotationPreferences =
            Windows::Graphics::Display::DisplayOrientations::Landscape;

        window->VisibilityChanged +=
            ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &View::OnVisibilityChanged);

        window->Closed +=
            ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &View::OnWindowClosed);

        window->PointerPressed +=
            ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &View::OnPointerPressed);

        window->PointerMoved +=
            ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &View::OnPointerMoved);

        window->PointerReleased +=
            ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &View::OnPointerReleased);

        InitD3D11(CoreWindow::GetForCurrentThread());
    }

    int checkLanguage()
    {
        ULONG numLanguages = 0;
        DWORD cchLanguagesBuffer = 0;
        BOOL hr = GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, NULL, &cchLanguagesBuffer);
        if (!hr) return 0;

        WCHAR* id = new WCHAR[cchLanguagesBuffer];
        hr = GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, id, &cchLanguagesBuffer);
        if (!hr) return 0;

        #define CHECK(str) (wcsstr(id, L##str L"-") != 0)

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

    virtual void Load(Platform::String^ entryPoint) {}

    virtual void Run()
    {
        cacheDir[0] = saveDir[0] = contentDir[0] = 0;

        StorageFolder^ localFolder = Windows::Storage::ApplicationData::Current->LocalFolder;
        wcstombs(cacheDir, localFolder->Path->Data(), sizeof(cacheDir));
        strcat(cacheDir, "\\");
        strcpy(saveDir, cacheDir);

        auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;
        const wchar_t* path = folder->Path->Data();
        wcstombs(contentDir, path, sizeof(contentDir) - 1);
        strcat(contentDir, "\\Content\\");

        osStartTime = osGetTimeMS();

        audioContext.start();

        Core::defLang = checkLanguage();

        Game::init((const char*)NULL);

        while (!Core::isQuit)
        {
            if (m_windowVisible)
            {
                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

                if (Game::update())
                {
                    Game::render();
                    Present();
                }
            }
            else
            {
                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
            }
        }

        audioContext.stop();
        Game::deinit();
    }

    virtual void Uninitialize() {}

protected:

    void OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
    {
        CoreWindow::GetForCurrentThread()->Activate();
    }

    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args)
    {
        audioContext.suspend();

        ReleaseResourcesForSuspending();

        Windows::ApplicationModel::SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();
        create_task([this, deferral]()
        {
            Game::quickSave();
            deferral->Complete();
        });
    }

    void OnResuming(Platform::Object^ sender, Platform::Object^ args)
    {
        CreateWindowSizeDependentResources();

        Game::quickLoad(true);

        audioContext.resume();
    }

    void OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
    {
        Core::quit();
    }

    void OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
    {
        m_windowVisible = args->Visible;
    }

    void OnPointer(Windows::UI::Input::PointerPoint^ p, bool down)
    {
        InputKey key = Input::getTouch(p->PointerId);
        if (key == ikNone) return;

        int32 y = Core::width - int32(ConvertDipsToPixels(p->Position.X));
        int32 x = int32(ConvertDipsToPixels(p->Position.Y));

        Input::setPos(key, vec2(float(x), float(y)));
        Input::setDown(key, down);
    }

    void OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
    {
        OnPointer(args->CurrentPoint, true);
    }

    void OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
    {
        OnPointer(args->CurrentPoint, true);
    }

    void OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
    {
        OnPointer(args->CurrentPoint, false);
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
int main(Platform::Array<Platform::String^>^)
{
    auto viewSource = ref new ViewSource();
    ApplicationModel::Core::CoreApplication::Run(viewSource);
    return 0;
}