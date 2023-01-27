#pragma once

#include <DwmThumbnail.h>
#include <Windows.h>
#include <winstring.h>
//#include <windows.ui.xaml.h>
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
#using "Microsoft.Graphics.Canvas.winmd"
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
using namespace Microsoft::Graphics::Canvas::Effects;
using namespace Windows::Graphics;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::Capture;
using namespace Windows::UI::ViewManagement;
using namespace Microsoft::UI::Windowing;
using namespace concurrency;

#define var auto

namespace TenMica
{
    private enum class FullScreenType //FULL_SCREEN_TYPE
    {
        Standard = 0x0,
        Minimal = 0x1,
        SuppressSystemOverlays = 0x2,
        None = 0x3,
    };

    [uuid("42a17e3d-7171-439a-b1fa-a31b7b957489")]
    private interface class IInternalCoreWindow
    {
        property MouseDevice^ MouseDevice { ::MouseDevice^ get(); }
        property int ApplicationViewState { int get(); }
        property int ApplicationViewOrientation { int get(); }
        property int AdjacentDisplayEdges { int get(); }
        property bool IsOnLockScreen{ bool get(); }
        property PointerVisualizationSettings^ PointerVisualizationSettings { ::PointerVisualizationSettings^ get(); }
        property CoreWindowResizeManager^ CoreWindowResizeManager { ::CoreWindowResizeManager^ get(); }
        property bool IsScreenCaptureEnabled;
        property FullScreenType SuppressSystemOverlays;
        event TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>^ ThemeChanged;
        event TypedEventHandler<CoreWindow^, Object^>^ ContextMenuRequested;
        event TypedEventHandler<CoreWindow^, Object^>^ DisplayChanged;
        event TypedEventHandler<CoreWindow^, Object^>^ Consolidated;
    };

    [uuid("c12779d8-85d2-43e5-901a-95dd4f8ecba3")]
    private interface class IInternalCoreWindow2
    {
        property Rect LayoutBounds { Rect get(); }
        property Rect VisibleBounds { Rect get(); }
        property ApplicationViewBoundsMode DesiredBoundsMode { ApplicationViewBoundsMode get(); }
        bool SetDesiredBoundsMode(ApplicationViewBoundsMode mode);
        void OnVisibleBoundsChange();
        event TypedEventHandler<IInternalCoreWindow2^, Platform::Object^>^ LayoutBoundsChanged;
        event TypedEventHandler<IInternalCoreWindow2^, Platform::Object^>^ VisibleBoundsChanged;
        event TypedEventHandler<IInternalCoreWindow2^, KeyEventArgs^>^ SysKeyDown;
        event TypedEventHandler<IInternalCoreWindow2^, KeyEventArgs^>^ SysKeyUp;
        event TypedEventHandler<IInternalCoreWindow2^, Platform::Object^>^ WindowPositionChanged;
        event TypedEventHandler<IInternalCoreWindow2^, Platform::Object^>^ SettingChanged;
        event TypedEventHandler<IInternalCoreWindow2^, Platform::Object^>^ ViewStateChanged;
        event TypedEventHandler<IInternalCoreWindow2^, CoreWindowEventArgs^>^ Destroying;
    };

    [uuid("FE54DDBE-32CF-5EE0-84BC-3EF7FAEAD1C6")]
    private interface class ISystemVisualProxyVisualPrivate
    {

    };

    DECLARE_INTERFACE_IID_(ISystemVisualProxyVisualPrivateInterop, IUnknown, "B2CFCBC2-7133-4EF8-A686-DB7FD4D536B4")
    {
        STDMETHOD(GetHandle)(THIS_
            OUT void** handle) PURE;
    };

    [uuid("6efeef10-e0c5-5997-bcb7-c1644f1cab81")]
    private interface class ISystemVisualProxyVisualPrivateStatics
    {
        ISystemVisualProxyVisualPrivate^ Create(Compositor^ compositor);
    };

    DECLARE_INTERFACE_IID_(ISystemVisualProxyVisualPrivateStaticsRaw, IInspectable, "6efeef10-e0c5-5997-bcb7-c1644f1cab81")
    {
        STDMETHOD(Create)(THIS_
            IN Compositor^ compositor,
            OUT ISystemVisualProxyVisualPrivate^* proxy) PURE;
    };

    DECLARE_INTERFACE_IID_(IVisualTargetPartner, IUnknown, "A1BEA8BA-D726-4663-8129-6B5E7927FFA6")
    {
        STDMETHOD(GetRoot)(THIS_
            OUT ABI::Windows::UI::Composition::IVisual** root) PURE;
        STDMETHOD(SetRoot)(THIS_
            OUT ABI::Windows::UI::Composition::IVisual* root) PURE;
    };

    DECLARE_INTERFACE_IID_(ICompositorPartner, IUnknown, "9CBD9312-070d-4588-9bf3-bbf528cf3e84")
    {
        STDMETHOD(CreateCursorVisual)(THIS_) PURE; //DUMMY
        STDMETHOD(CreateSharedTarget)(THIS_) PURE; //DUMMY
        STDMETHOD(CreateSharedVisual)(THIS_) PURE; //DUMMY
        STDMETHOD(HintSize)(THIS_) PURE; //DUMMY
        STDMETHOD(OfferSurfaceResources)(THIS_) PURE; //DUMMY
        STDMETHOD(OpenSharedResourceHandle)(THIS_) PURE; //DUMMY
        STDMETHOD(OpenSharedTargetFromHandle)(THIS_
            IN void* handle, OUT IVisualTargetPartner** target) PURE;
    };

    [uuid("bb683713-bb45-50c7-8195-9af24dcc62f6")]
    private interface class IVisualInternal
    {
        property float RasterizationScaleOverride;
        IAsyncOperation<ICompositionSurface^>^ CaptureAsync(Visual^ visual, CompositionGraphicsDevice^ graphicsDevice, int w, int h, DirectXPixelFormat pixelFormat, DirectXAlphaMode alphaMode);
    };

    //TODO: inherit from IXamlCompositionBrushBaseOverridesPrivate to support non-full-window Mica scenarios
    //TODO: add support for AppWindow, do not assume CoreWindow
    //TODO: add multi-monitor support
    //TODO: handle theming properly

    public ref class TenMicaBrush sealed : XamlCompositionBrushBase
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

        CompositionVisualSurface^ surface;
        SpriteVisual^ spriteVisual;
        CompositionSurfaceBrush^ theBrush;
        CompositionGraphicsDevice^ theGraphicsDevice;
        Direct3D11CaptureFramePool^ pool;
        GraphicsCaptureSession^ gS;
        Windows::UI::Composition::Compositor^ winComp;
        Window^ cWindow;
        Microsoft::UI::Windowing::AppWindow^ appWindow;
        HWND cHwnd { 0 };

        Windows::Foundation::EventRegistrationToken OnActivatedCookie;
        Windows::Foundation::EventRegistrationToken OnColorValuesChangedCookie;
        Windows::Foundation::EventRegistrationToken OnHighContrastChangedCookie;
        Windows::Foundation::EventRegistrationToken OnEnergySaverStatusChangedCookie;
        Windows::Foundation::EventRegistrationToken OnCompositionCapabilitiesChangedCookie;
        Windows::Foundation::EventRegistrationToken OnWindowPositionChangedCookie;
        Windows::Foundation::EventRegistrationToken OnDisplayChangedCookie;
        Windows::Foundation::EventRegistrationToken FrameArrivedCookie;

        ComPtr<ID3D11Device> direct3dDevice;
        ComPtr<IDXGIDevice2> dxgiDevice;
        ComPtr<ID2D1Factory2> d2dFactory2;
        ComPtr<ID2D1Device> d2dDevice;
        //ComPtr<IDCompositionDesktopDevice> dcompDevice;
        //ComPtr<IVisualTargetPartner> cTarget;

        void Init();
        bool CreateDevice();
        ::CompositionBrush^ BuildMicaEffectBrush(Compositor^ compositor, Visual^ src, Color tintColor, float tintOpacity, float luminosityOpacity, SIZE size);
        void UpdateVisual(RECT rect);
        ::CompositionBrush^ CreateCrossFadeEffectBrush(Compositor^ compositor, ::CompositionBrush^ from, ::CompositionBrush^ to);
        ScalarKeyFrameAnimation^ CreateCrossFadeAnimation(Compositor^ compositor);
        void UpdateBrush();
        void OnActivated(Object^ sender, Microsoft::UI::Xaml::WindowActivatedEventArgs^ args);
        void OnColorValuesChanged(UISettings^ sender, Object^ args);
        void OnHighContrastChanged(AccessibilitySettings^ sender, Platform::Object^ args);
        void OnEnergySaverStatusChanged(Platform::Object^ sender, Platform::Object^ e);
        void OnCompositionCapabilitiesChanged(CompositionCapabilities^ sender, Platform::Object^ args);
        void OnWindowPositionChanged(IInternalCoreWindow2^ window, Platform::Object^ args);
        void OnDisplayChanged(Windows::UI::Core::CoreWindow^ sender, Platform::Object^ args);
        void OnAppWindowChanged(Microsoft::UI::Windowing::AppWindow^ sender, AppWindowChangedEventArgs^ args);
        Windows::UI::Composition::Compositor^ TenMicaBrush::InitializeInteropCompositor(IUnknown* d2dDevice);
    public:
        TenMicaBrush();
        TenMicaBrush(ApplicationTheme ForcedTheme);

        property bool IsThemeForced { bool get(); void set(bool value); }
        property bool EnabledInActivatedNotForeground { bool get(); void set(bool value); }
        property ApplicationTheme ForcedTheme { ApplicationTheme get(); void set(ApplicationTheme value); }
        property Window^ TargetWindow;
    protected:
        virtual void OnConnected() override;
        virtual void OnDisconnected() override;
    };
}
