#pragma once

#include <DwmThumbnail.h>
#include <winstring.h>
#include <windows.ui.xaml.h>
#include <wrl.h>
#include <ntstatus.h>
#include <winternl.h>
#include <comutil.h>
#include <dcomp.h>
#include <dwmapi.h>
#include <corewindow.h>
#include <windows.ui.core.h>
#include <WindowsNumerics.h>

using namespace Platform;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Devices::Input;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Core;
using namespace Windows::UI;
using namespace Windows::UI::Input;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Microsoft::Graphics::Canvas::Effects;
using namespace Windows::UI::ViewManagement;

#define var auto

#pragma pack (8) // C++/CX is stupid and won't allow us to compile on x86/Win32 without this...

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
        GetWindowRectProto GetWindowRect;
        FindWindowWProto FindWindow;
        GetParentProto GetParent;
        SetWindowCompositionAttribute lSetWindowCompositionAttribute;

        ApplicationTheme forcedTheme;
        bool isThemeForced = false;
        bool fastEffects;
        bool energySaver;
        UISettings^ uiSettings;
        AccessibilitySettings^ accessibilitySettings;
        bool windowActivated;
        bool enableInActivatedNotForeground = false;
        HWND hwndHelper;

        CompositionVisualSurface^ surface;
        CoreWindow^ cWindow;
        HWND cHwnd;

        Windows::Foundation::EventRegistrationToken OnActivatedCookie;
        Windows::Foundation::EventRegistrationToken OnColorValuesChangedCookie;
        Windows::Foundation::EventRegistrationToken OnHighContrastChangedCookie;
        Windows::Foundation::EventRegistrationToken OnEnergySaverStatusChangedCookie;
        Windows::Foundation::EventRegistrationToken OnCompositionCapabilitiesChangedCookie;
        Windows::Foundation::EventRegistrationToken OnWindowPositionChangedCookie;
        Windows::Foundation::EventRegistrationToken OnDisplayChangedCookie;

        void Init();
        ::CompositionBrush^ BuildMicaEffectBrush(Compositor^ compositor, Visual^ src, Color tintColor, float tintOpacity, float luminosityOpacity, SIZE size);
        void UpdateVisual(RECT rect);
        ::CompositionBrush^ CreateCrossFadeEffectBrush(Compositor^ compositor, ::CompositionBrush^ from, ::CompositionBrush^ to);
        ScalarKeyFrameAnimation^ CreateCrossFadeAnimation(Compositor^ compositor);
        void UpdateBrush();
        void OnActivated(CoreWindow^ sender, WindowActivatedEventArgs^ args);
        void OnColorValuesChanged(UISettings^ sender, Object^ args);
        void OnHighContrastChanged(AccessibilitySettings^ sender, Platform::Object^ args);
        void OnEnergySaverStatusChanged(Platform::Object^ sender, Platform::Object^ e);
        void OnCompositionCapabilitiesChanged(CompositionCapabilities^ sender, Platform::Object^ args);
        void OnWindowPositionChanged(IInternalCoreWindow2^ window, Platform::Object^ args);
        void OnDisplayChanged(Windows::UI::Core::CoreWindow^ sender, Platform::Object^ args);
    public:
        TenMicaBrush();
        TenMicaBrush(ApplicationTheme ForcedTheme);

        property bool IsThemeForced { bool get(); void set(bool value); }
        property bool EnabledInActivatedNotForeground { bool get(); void set(bool value); }
        property ApplicationTheme ForcedTheme { ApplicationTheme get(); void set(ApplicationTheme value); }
    protected:
        virtual void OnConnected() override;
        virtual void OnDisconnected() override;
    };
}
