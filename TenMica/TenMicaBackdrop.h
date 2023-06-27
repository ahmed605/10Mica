#pragma once

#include <DwmThumbnail.h>
#include <Windows.h>
#include <winstring.h>
#include <microsoft.ui.xaml.window.h>
#include <wrl.h>
#include <ntstatus.h>
#include <winternl.h>
#include <comutil.h>
#include <dcomp.h>
#include <dwmapi.h>
#include <corewindow.h>
#include <windows.ui.core.h>
#include <WindowsNumerics.h>
#include <ppltasks.h>
#include <pplawait.h>
#include <experimental\resumable>

#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include <d2d1_2helper.h>

#include <microsoft.ui.composition.interop.h>

#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dcomp")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "ole32")

#using "Microsoft.UI.winmd"
#using "Microsoft.UI.Xaml.winmd"
#using "Microsoft.Graphics.winmd"

using namespace Platform;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Devices::Input;
using namespace Microsoft::UI::Xaml;
using namespace Windows::UI::Core;
using namespace Windows::UI;
using namespace Windows::UI::Input;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Private::Composition::Effects;
using namespace Windows::Graphics;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::Capture;
using namespace Windows::UI::ViewManagement;
using namespace Microsoft::UI::Windowing;
using namespace concurrency;

#define var auto

namespace TenMica
{
    //TODO: add multi-monitor support
    //TODO: handle theming properly

    public ref class TenMicaBackdrop sealed : SystemBackdrop
    {
    private:
        DwmpQueryThumbnailType lDwmpQueryThumbnailType;
        DwmpCreateSharedThumbnailVisual lDwmpCreateSharedThumbnailVisual;
        DwmpQueryWindowThumbnailSourceSize lDwmpQueryWindowThumbnailSourceSize;
        SetWindowCompositionAttribute lSetWindowCompositionAttribute;

        ApplicationTheme forcedTheme;
        bool isThemeForced = false;
        bool fastEffects;
        bool energySaver;
        UISettings^ uiSettings;
        AccessibilitySettings^ accessibilitySettings;
        CompositionCapabilities^ compCapabilities;
        bool windowActivated;
        bool enableInActivatedNotForeground = false;
        HWND hwndHelper;

        Windows::UI::Composition::CompositionVisualSurface^ surface;
        Windows::UI::Composition::Compositor^ winComp;
        Microsoft::UI::Windowing::AppWindow^ appWindow;
        Windows::UI::Composition::Visual^ wndVisual;
        ICompositionSupportsSystemBackdrop^ backdrop;
        property SystemBackdrops::SystemBackdropConfiguration^ backdropConfig;
        XamlRoot^ xamlRoot;

        HWND cHwnd{ 0 };
        //Window^ cWindow;
        HTHUMBNAIL hThumbWindow = NULL;

        //Windows::Foundation::EventRegistrationToken OnActivatedCookie;
        Windows::Foundation::EventRegistrationToken OnColorValuesChangedCookie;
        Windows::Foundation::EventRegistrationToken OnHighContrastChangedCookie;
        Windows::Foundation::EventRegistrationToken OnEnergySaverStatusChangedCookie;
        Windows::Foundation::EventRegistrationToken OnCompositionCapabilitiesChangedCookie;
        Windows::Foundation::EventRegistrationToken OnWindowPositionChangedCookie;
        //Windows::Foundation::EventRegistrationToken OnDisplayChangedCookie;
        //Windows::Foundation::EventRegistrationToken FrameArrivedCookie;

        ComPtr<ID3D11Device> direct3dDevice;
        ComPtr<IDXGIDevice2> dxgiDevice;
        ComPtr<ID2D1Factory2> d2dFactory2;
        ComPtr<ID2D1Device> d2dDevice;

        void Init();
        bool CreateDevice();
        Windows::UI::Composition::CompositionBrush^ BuildMicaEffectBrush(Windows::UI::Composition::Compositor^ compositor, Windows::UI::Composition::Visual^ src, Color tintColor, float tintOpacity, float luminosityOpacity, SIZE size);
        void UpdateVisual(RECT rect);
        Windows::UI::Composition::CompositionBrush^ CreateCrossFadeEffectBrush(Windows::UI::Composition::Compositor^ compositor, Windows::UI::Composition::CompositionBrush^ from, Windows::UI::Composition::CompositionBrush^ to);
        Windows::UI::Composition::ScalarKeyFrameAnimation^ CreateCrossFadeAnimation(Windows::UI::Composition::Compositor^ compositor);
        void UpdateBrush();
        //void OnActivated(Object^ sender, Microsoft::UI::Xaml::WindowActivatedEventArgs^ args);
        void OnColorValuesChanged(UISettings^ sender, Object^ args);
        void OnHighContrastChanged(AccessibilitySettings^ sender, Platform::Object^ args);
        void OnEnergySaverStatusChanged(Platform::Object^ sender, Platform::Object^ e);
        void OnCompositionCapabilitiesChanged(CompositionCapabilities^ sender, Platform::Object^ args);
        //void OnWindowPositionChanged(IInternalCoreWindow2^ window, Platform::Object^ args);
        //void OnDisplayChanged(Windows::UI::Core::CoreWindow^ sender, Platform::Object^ args);
        void OnAppWindowChanged(Microsoft::UI::Windowing::AppWindow^ sender, AppWindowChangedEventArgs^ args);
        Windows::UI::Composition::Compositor^ TenMicaBackdrop::InitializeInteropCompositor(IUnknown* d2dDevice);
    public:
        TenMicaBackdrop();
        TenMicaBackdrop(ApplicationTheme ForcedTheme);

        property bool IsThemeForced { bool get(); void set(bool value); }
        property bool EnabledInActivatedNotForeground { bool get(); void set(bool value); }
        property ApplicationTheme ForcedTheme { ApplicationTheme get(); void set(ApplicationTheme value); }
    protected:
        virtual void OnTargetConnected(ICompositionSupportsSystemBackdrop^ connectedTarget, XamlRoot^ xamlRoot) override;
        virtual void OnTargetDisconnected(ICompositionSupportsSystemBackdrop^ disconnectedTarget) override;
        virtual void OnDefaultSystemBackdropConfigurationChanged(ICompositionSupportsSystemBackdrop^ target, XamlRoot^ xamlRoot) override;
    };
}

[uuid("f2aa238f-1c21-581e-aadc-7d6ec5320f56")]
interface class IDesktopWindowXamlSource
{
    property UIElement^ Content;
    property bool HasFocus { bool get(); }
    property SystemBackdrop^ SystemBackdrop;
    property Platform::Object^ SiteBridge { Platform::Object^ get(); }
};

[uuid("4258422d-2fcf-5f11-8d28-b558a1fbed51")]
interface class IDesktopSiteBridge
{
    property bool InputEnabled;
    property Platform::Object^ WindowBridge { Platform::Object^ get(); }
    property Microsoft::UI::WindowId WindowId { Microsoft::UI::WindowId get(); }
};