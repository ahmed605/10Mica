#include "pch.h"
#include "TenMicaBackdrop.h"
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Graphics::Capture;
using namespace Windows::ApplicationModel;
using namespace Windows::System::Power;

using namespace TenMica;
using namespace Platform;

TenMicaBackdrop::TenMicaBackdrop() { Init(); }

TenMicaBackdrop::TenMicaBackdrop(ApplicationTheme ForcedTheme) : forcedTheme(ForcedTheme), isThemeForced(true) { Init(); }

void TenMicaBackdrop::Init()
{
	auto dwmapiLib = LoadLibrary(L"dwmapi.dll");
	auto user32 = GetModuleHandle(L"user32.dll");

	lDwmpQueryThumbnailType = (DwmpQueryThumbnailType)GetProcAddress(dwmapiLib, MAKEINTRESOURCEA(114));
	lDwmpCreateSharedThumbnailVisual = (DwmpCreateSharedThumbnailVisual)GetProcAddress(dwmapiLib, MAKEINTRESOURCEA(147));
	lDwmpQueryWindowThumbnailSourceSize = (DwmpQueryWindowThumbnailSourceSize)GetProcAddress(dwmapiLib, MAKEINTRESOURCEA(162));

	lSetWindowCompositionAttribute = (SetWindowCompositionAttribute)GetProcAddress(user32, "SetWindowCompositionAttribute");
}

Windows::UI::Composition::CompositionBrush^ TenMicaBackdrop::BuildMicaEffectBrush(Windows::UI::Composition::Compositor^ compositor, Windows::UI::Composition::Visual^ src, Color tintColor, float tintOpacity, float luminosityOpacity, SIZE size)
{
	// Tint Color.

	var tintColorEffect = ref new ColorSourceEffect();
	tintColorEffect->Name = "TintColor";
	tintColorEffect->Color = tintColor;

	// OpacityEffect applied to Tint.
	var tintOpacityEffect = ref new OpacityEffect();
	tintOpacityEffect->Name = "TintOpacity";
	tintOpacityEffect->Opacity = tintOpacity;
	tintOpacityEffect->Source = tintColorEffect;

	// Apply Luminosity:

	// Luminosity Color.
	var luminosityColorEffect = ref new ColorSourceEffect();
	luminosityColorEffect->Color = tintColor;

	// OpacityEffect applied to Luminosity.
	var luminosityOpacityEffect = ref new OpacityEffect();
	luminosityOpacityEffect->Name = "LuminosityOpacity";
	luminosityOpacityEffect->Opacity = luminosityOpacity;
	luminosityOpacityEffect->Source = luminosityColorEffect;

	// Luminosity Blend.
	var luminosityBlendEffect = ref new BlendEffect();
	luminosityBlendEffect->Mode = BlendEffectMode::Color;
	luminosityBlendEffect->Background = ref new Windows::UI::Composition::CompositionEffectSourceParameter("BlurredWallpaperBackdrop");
	luminosityBlendEffect->Foreground = luminosityOpacityEffect;

	// Apply Tint:

	// Color Blend.
	var colorBlendEffect = ref new BlendEffect();
	colorBlendEffect->Mode = BlendEffectMode::Luminosity;
	colorBlendEffect->Background = luminosityBlendEffect;
	colorBlendEffect->Foreground = tintOpacityEffect;

	Windows::UI::Composition::CompositionEffectBrush^ micaEffectBrush = compositor->CreateEffectFactory(colorBlendEffect)->CreateBrush();

	var srcSize = Windows::Foundation::Numerics::float2::float2(size.cx, size.cy);

	if (surface != nullptr)
	{
		try
		{
			Windows::UI::Composition::Visual^ visual;
			visual = surface->SourceVisual;

			surface->SourceVisual = nullptr;
			delete surface;
			surface = nullptr;
			delete visual;
			visual = nullptr;
		} catch (...) { }
	}

	src->Size = srcSize;

	surface = compositor->CreateVisualSurface();
	surface->SourceVisual = src;
	surface->SourceSize = srcSize;

	var brushRaw = compositor->CreateSurfaceBrush(surface);
	brushRaw->Stretch = Windows::UI::Composition::CompositionStretch::None;


	// Blur
	var blurEffect = ref new GaussianBlurEffect();
	blurEffect->Name = "Blur";
	blurEffect->BlurAmount = 140.0f;
	blurEffect->Source = ref new Windows::UI::Composition::CompositionEffectSourceParameter("src");

	var brush = compositor->CreateEffectFactory(blurEffect)->CreateBrush();
	brush->SetSourceParameter("src", brushRaw);

	micaEffectBrush->SetSourceParameter("BlurredWallpaperBackdrop", brush);

	return micaEffectBrush;
}

// Ugly hack, basically
void TenMicaBackdrop::UpdateVisual(RECT rect)
{
	surface->SourceSize = Windows::Foundation::Numerics::float2::float2(rect.right - rect.left, rect.bottom - rect.top);
	surface->SourceOffset = Windows::Foundation::Numerics::float2::float2(rect.left, rect.top);
}

#define null nullptr

typedef HRESULT(STDAPICALLTYPE* PFNGETACTIVATIONFACTORY2)(HSTRING, void**);

void TenMicaBackdrop::OnTargetConnected(ICompositionSupportsSystemBackdrop^ connectedTarget, XamlRoot^ xamlRootObj)
{
	if (connectedTarget->SystemBackdrop != nullptr) return;

	if (DesignMode::DesignModeEnabled) return;

	backdropConfig = GetDefaultSystemBackdropConfiguration(connectedTarget, xamlRootObj);
	backdrop = connectedTarget;
	xamlRoot = xamlRootObj;

	auto xamlSource = (IDesktopWindowXamlSource^)connectedTarget;
	auto siteBridge = (IDesktopSiteBridge^)xamlSource->SiteBridge;

	if (uiSettings == null)
	{
		uiSettings = ref new UISettings();
	}

	if (accessibilitySettings == null)
	{
		accessibilitySettings = ref new AccessibilitySettings();
	}

	compCapabilities = ref new CompositionCapabilities();

	fastEffects = compCapabilities->AreEffectsFast();

	energySaver = PowerManager::EnergySaverStatus == EnergySaverStatus::On;

	//windowActivated = backdropConfig->IsInputActive || enableInActivatedNotForeground;
	windowActivated = true;

	//ComPtr<IWindowNative> coreWndRaw;
	///((IUnknown*)cWindow)->QueryInterface(coreWndRaw.GetAddressOf());

	// Getting our HWND
	//coreWndRaw->get_WindowHandle(&cHwnd);

	cHwnd = GetParent((HWND)siteBridge->WindowId.Value);
	appWindow = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(Microsoft::UI::WindowId{ (unsigned long long)cHwnd });

	CreateDevice();
	winComp = InitializeInteropCompositor(d2dDevice.Get());

	HWND targetWindow = (HWND)FindWindow(L"Progman", NULL); // Progman is still the name of the Desktop window :))))

	SendMessage(targetWindow,
		0x052C,
		0x0000000D,
		0);

	SendMessage(targetWindow,
		0x052C,
		0x0000000D,
		1);

	SendMessageTimeout(targetWindow, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);

	SIZE windowSize{};
	lDwmpQueryWindowThumbnailSourceSize(targetWindow, FALSE, &windowSize);

	// Officially documented
	DWM_THUMBNAIL_PROPERTIES thumb{};
	thumb.dwFlags = DWM_TNP_SOURCECLIENTAREAONLY | DWM_TNP_VISIBLE | DWM_TNP_RECTDESTINATION | DWM_TNP_RECTSOURCE | DWM_TNP_OPACITY | DWM_TNP_ENABLE3D;
	thumb.opacity = 255;
	thumb.fVisible = TRUE;
	thumb.fSourceClientAreaOnly = TRUE;
	thumb.rcDestination = RECT{ 0, 0, windowSize.cx, windowSize.cy };
	thumb.rcSource = RECT{ 0, 0, windowSize.cx, windowSize.cy };

	ComPtr<IDCompositionDevice3> dcompDevice;
	ComPtr<IDCompositionVisual2> windowVisual;

	(reinterpret_cast<IInspectable*>(winComp))->QueryInterface(dcompDevice.GetAddressOf());

	//TODO: check if comp implements IDCompositionDevice before calling, it doesn't implement it if we aren't running under XAML or having no InteropCompositor
	auto result = lDwmpCreateSharedThumbnailVisual(cHwnd, targetWindow, 2, &thumb, dcompDevice.Get(), (void**)windowVisual.GetAddressOf(), &hThumbWindow);

	// you can cast directly but this is safer, like in case we don't have an InteropCompositor
	ComPtr<ABI::Windows::UI::Composition::IVisual> visualRaw;
	windowVisual->QueryInterface(visualRaw.GetAddressOf());
	wndVisual = reinterpret_cast<Windows::UI::Composition::Visual^>(visualRaw.Get());

	winComp->RequestCommitAsync();

	UpdateBrush();

	//OnActivatedCookie = cWindow->Activated += ref new Windows::Foundation::TypedEventHandler<Object^, Microsoft::UI::Xaml::WindowActivatedEventArgs^>(this, &TenMica::TenMicaBackdrop::OnActivated);

	OnColorValuesChangedCookie = uiSettings->ColorValuesChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::ViewManagement::UISettings^, Platform::Object^>(this, &TenMica::TenMicaBackdrop::OnColorValuesChanged);
	//OnHighContrastChangedCookie = accessibilitySettings->HighContrastChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::ViewManagement::AccessibilitySettings^, Platform::Object^>(this, &TenMica::TenMicaBackdrop::OnHighContrastChanged);
	OnCompositionCapabilitiesChangedCookie = compCapabilities->Changed += ref new Windows::Foundation::TypedEventHandler<Microsoft::UI::Composition::CompositionCapabilities^, Platform::Object^>(this, &TenMica::TenMicaBackdrop::OnCompositionCapabilitiesChanged);
	OnEnergySaverStatusChangedCookie = PowerManager::EnergySaverStatusChanged += ref new Windows::Foundation::EventHandler<Platform::Object^>(this, &TenMica::TenMicaBackdrop::OnEnergySaverStatusChanged);

	//IInternalCoreWindow2^ internalW = (IInternalCoreWindow2^)coreWnd; // Pure black magic

	// Syncing!!
	//OnWindowPositionChangedCookie = internalW->WindowPositionChanged += ref new TypedEventHandler<IInternalCoreWindow2^, Platform::Object^>(this, &TenMicaBackdrop::OnWindowPositionChanged);

	OnWindowPositionChangedCookie = appWindow->Changed += ref new TypedEventHandler<Microsoft::UI::Windowing::AppWindow^, AppWindowChangedEventArgs^>(this, &TenMica::TenMicaBackdrop::OnAppWindowChanged);

	//IInternalCoreWindow^ internalW1 = (IInternalCoreWindow^)coreWnd; // Pure blue magic
	//OnDisplayChangedCookie = internalW1->DisplayChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Platform::Object^>(this, &TenMica::TenMicaBackdrop::OnDisplayChanged);
}

bool TenMicaBackdrop::CreateDevice()
{
	if (D3D11CreateDevice(0, //Adapter
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		NULL,
		0,
		D3D11_SDK_VERSION,
		direct3dDevice.GetAddressOf(),
		nullptr,
		nullptr
	) != S_OK)
	{
		//Maybe try creating with D3D_DRIVER_TYPE_WARP before returning false.
		//Always listen to device changes.
		return false;
	}

	if (direct3dDevice->QueryInterface(dxgiDevice.GetAddressOf()) != S_OK)
	{
		return false;
	}

	if (D2D1CreateFactory(
		D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_SINGLE_THREADED,
		__uuidof(ID2D1Factory2),
		(void**)d2dFactory2.GetAddressOf()) != S_OK)
	{
		return false;
	}

	if (d2dFactory2->CreateDevice(
		dxgiDevice.Get(),
		d2dDevice.GetAddressOf()) != S_OK)
	{
		return false;
	}

	return true;
}

Windows::UI::Composition::Compositor^ TenMicaBackdrop::InitializeInteropCompositor(IUnknown* d2dDevice)
{
	ComPtr<IInteropCompositorFactoryPartner> interopCompositorFactory;
	GetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(L"Windows.UI.Composition.Compositor").Get(), interopCompositorFactory.GetAddressOf());

	ComPtr<IInteropCompositorPartner> interopCompositor;
	auto interopRes = interopCompositorFactory->CreateInteropCompositor(d2dDevice, NULL, __uuidof(IInteropCompositorPartner), (void**)interopCompositor.GetAddressOf());
	if (interopRes != S_OK)
		return null;

	ComPtr<ABI::Windows::UI::Composition::ICompositor> comp;
	interopCompositor->QueryInterface(comp.GetAddressOf());

	return reinterpret_cast<Windows::UI::Composition::Compositor^>(comp.Get());
}

void TenMicaBackdrop::OnAppWindowChanged(Microsoft::UI::Windowing::AppWindow^ sender, AppWindowChangedEventArgs^ args)
{
	if (surface != nullptr && args->DidPositionChange)
	{
		RECT rect;
		GetWindowRect(cHwnd, &rect);
		UpdateVisual(rect);
	}
}

/*void TenMicaBackdrop::OnDisplayChanged(Windows::UI::Core::CoreWindow^ sender, Platform::Object^ args)
{
	cWindow->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this, sender]()
	{
		UpdateBrush();
	}));
}*/

void TenMicaBackdrop::OnWindowPositionChanged(IInternalCoreWindow2^ window, Platform::Object^ args)
{
	if (surface != nullptr)
	{
		RECT rect;
		GetWindowRect(cHwnd, &rect);
		UpdateVisual(rect);
	}
}

void TenMicaBackdrop::OnColorValuesChanged(UISettings^ sender, Object^ args)
{
	xamlRoot->Content->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this, sender]()
	{
		UpdateBrush();
	}));
}

void TenMicaBackdrop::OnHighContrastChanged(AccessibilitySettings^ sender, Platform::Object^ args)
{
	xamlRoot->Content->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this, sender]()
	{
		UpdateBrush();
	}));
}

void TenMicaBackdrop::OnEnergySaverStatusChanged(Platform::Object^ sender, Platform::Object^ e)
{
	xamlRoot->Content->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this, sender]()
	{
		energySaver = PowerManager::EnergySaverStatus == EnergySaverStatus::On;
		UpdateBrush();
	}));
}

void TenMicaBackdrop::OnCompositionCapabilitiesChanged(CompositionCapabilities^ sender, Platform::Object^ args)
{
	xamlRoot->Content->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this, sender]()
	{
		fastEffects = compCapabilities->AreEffectsFast();
		UpdateBrush();
	}));
}

void TenMicaBackdrop::OnActivated(Object^ sender, Microsoft::UI::Xaml::WindowActivatedEventArgs^ args)
{
	((Window^)sender)->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this, sender, args]()
	{
		var activated = args->WindowActivationState == WindowActivationState::PointerActivated || args->WindowActivationState == WindowActivationState::CodeActivated
			|| (enableInActivatedNotForeground && args->WindowActivationState == WindowActivationState::Deactivated);
		if (windowActivated && activated == windowActivated)
		{
			return;
		}
		windowActivated = activated;
		UpdateBrush();
	}));
}

Windows::UI::Composition::CompositionBrush^ TenMicaBackdrop::CreateCrossFadeEffectBrush(Windows::UI::Composition::Compositor^ compositor, Windows::UI::Composition::CompositionBrush^ from, Windows::UI::Composition::CompositionBrush^ to)
{
	var crossFadeEffect = ref new CrossFadeEffect();
	crossFadeEffect->Name = "Crossfade"; // Name to reference when starting the animation.
	crossFadeEffect->Source1 = ref new Windows::UI::Composition::CompositionEffectSourceParameter("source1");
	crossFadeEffect->Source2 = ref new Windows::UI::Composition::CompositionEffectSourceParameter("source2");
	crossFadeEffect->Weight = 0;

	auto list = ref new Platform::Collections::Vector<String^>();
	list->Append("Crossfade.Weight");

	Windows::UI::Composition::CompositionEffectBrush^ crossFadeEffectBrush = compositor->CreateEffectFactory(crossFadeEffect, list)->CreateBrush();
	crossFadeEffectBrush->Comment = "Crossfade";
	crossFadeEffectBrush->SetSourceParameter("source1", from);
	crossFadeEffectBrush->SetSourceParameter("source2", to);
	return crossFadeEffectBrush;
}

Windows::UI::Composition::ScalarKeyFrameAnimation^ TenMicaBackdrop::CreateCrossFadeAnimation(Windows::UI::Composition::Compositor^ compositor)
{
	Windows::UI::Composition::ScalarKeyFrameAnimation^ animation = compositor->CreateScalarKeyFrameAnimation();
	Windows::UI::Composition::LinearEasingFunction^ linearEasing = compositor->CreateLinearEasingFunction();
	animation->InsertKeyFrame(0.0f, 0.0f, linearEasing);
	animation->InsertKeyFrame(1.0f, 1.0f, linearEasing);
	animation->Duration = TimeSpan{ 2500000 };
	return animation;
}

void TenMicaBackdrop::UpdateBrush()
{
	if (uiSettings == nullptr || accessibilitySettings == nullptr)
	{
		return;
	}

	bool useSolidColorFallback = !windowActivated || fastEffects == false || energySaver == true || !uiSettings->AdvancedEffectsEnabled;

	var compositor = winComp;

	var isLightMode = isThemeForced ? ForcedTheme == ApplicationTheme::Light : Application::Current->RequestedTheme == ApplicationTheme::Light;
	Color tintColor = isLightMode ? Color{ 255, 243, 243, 243 } : Color{ 255, 32, 32, 32 };
	float tintOpacity = isLightMode ? 0.5f : 0.8f;

	if (accessibilitySettings->HighContrast)
	{
		tintColor = uiSettings->GetColorValue(UIColorType::Background);
		useSolidColorFallback = true;
	}

	//FallbackColor = tintColor;

	Windows::UI::Composition::CompositionBrush^ newBrush;

	if (useSolidColorFallback)
	{
		newBrush = compositor->CreateColorBrush(tintColor);
	}
	else
	{
		HWND targetWindow = (HWND)FindWindow(L"Progman", NULL); // Progman is still the name of the Desktop window :))))

		SIZE windowSize{};
		lDwmpQueryWindowThumbnailSourceSize(targetWindow, FALSE, &windowSize);

		SendMessage(targetWindow,
			0x052C,
			0x0000000D,
			0);

		SendMessage(targetWindow,
			0x052C,
			0x0000000D,
			1);

		SendMessageTimeout(targetWindow, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);

		newBrush = BuildMicaEffectBrush(compositor, wndVisual, tintColor, tintOpacity, 1.0f, windowSize);

		RECT rect;
		GetWindowRect(cHwnd, &rect);
		UpdateVisual(rect);

		winComp->RequestCommitAsync();
		compositor->RequestCommitAsync();
	}

	Windows::UI::Composition::CompositionBrush^ oldBrush = backdrop->SystemBackdrop;

	if (oldBrush == nullptr || (oldBrush->Comment == "Crossfade") || (dynamic_cast<Windows::UI::Composition::CompositionColorBrush^>(oldBrush) != nullptr && dynamic_cast<Windows::UI::Composition::CompositionColorBrush^>(newBrush) != nullptr))
	{
		// Set new brush directly
		backdrop->SystemBackdrop = newBrush;

		if (oldBrush != nullptr)
		{
			delete oldBrush;
		}
	}
	else
	{
		// Crossfade
		Windows::UI::Composition::CompositionBrush^ crossFadeBrush = CreateCrossFadeEffectBrush(compositor, oldBrush, newBrush);
		Windows::UI::Composition::ScalarKeyFrameAnimation^ animation = CreateCrossFadeAnimation(compositor);
		backdrop->SystemBackdrop = crossFadeBrush;

		var crossFadeAnimationBatch = compositor->CreateScopedBatch(Windows::UI::Composition::CompositionBatchTypes::Animation);
		crossFadeBrush->StartAnimation("CrossFade.Weight", animation);
		crossFadeAnimationBatch->End();

		crossFadeAnimationBatch->Completed += ref new TypedEventHandler<Platform::Object^, Windows::UI::Composition::CompositionBatchCompletedEventArgs^>([this, crossFadeBrush, oldBrush, newBrush](Platform::Object^, Windows::UI::Composition::CompositionBatchCompletedEventArgs^)
			{
				backdrop->SystemBackdrop = newBrush;
				delete crossFadeBrush;
				delete oldBrush;
			});
	}
}

void TenMicaBackdrop::OnTargetDisconnected(ICompositionSupportsSystemBackdrop^ disconnectedTarget)
{
	//TODO: do proper disposing
	if (backdrop != nullptr)
	{
		try
		{
			delete backdrop;
			disconnectedTarget->SystemBackdrop = null;

			if (appWindow != null)
			{
				//cWindow->Activated -= OnActivatedCookie;
				appWindow->Changed -= OnWindowPositionChangedCookie;

				//cWindow = null;
				appWindow = null;
			}

			if (uiSettings != null)
			{
				uiSettings->ColorValuesChanged -= OnColorValuesChangedCookie;
				uiSettings = null;
			}

			if (accessibilitySettings != null)
			{
				//accessibilitySettings->HighContrastChanged -= OnHighContrastChangedCookie;
				accessibilitySettings = null;
			}

			if (hThumbWindow) DwmUnregisterThumbnail(hThumbWindow);

			compCapabilities->Changed -= OnCompositionCapabilitiesChangedCookie;
			compCapabilities = null;

			PowerManager::EnergySaverStatusChanged -= OnEnergySaverStatusChangedCookie;

			Windows::UI::Composition::Visual^ visual;
			visual = surface->SourceVisual;

			surface->SourceVisual = null;
			delete surface;
			surface = null;

			d2dDevice->Release();
			dxgiDevice->Release();
			direct3dDevice->Release();
			d2dFactory2->Release();

			delete visual;
			visual = null;

			delete winComp;
			winComp = null;
		} catch (...) { }
	}
}

void TenMicaBackdrop::OnDefaultSystemBackdropConfigurationChanged(ICompositionSupportsSystemBackdrop^ target, XamlRoot^ xamlRootObj)
{
	backdropConfig = GetDefaultSystemBackdropConfiguration(target, xamlRootObj);
	xamlRootObj->Content->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this]()
		{
			var activated = backdropConfig->IsInputActive;
			if (windowActivated && activated == windowActivated)
			{
				return;
			}
			windowActivated = activated;
			UpdateBrush();
		}));
}

ApplicationTheme TenMicaBackdrop::ForcedTheme::get() { return forcedTheme; }

void TenMicaBackdrop::ForcedTheme::set(ApplicationTheme value)
{
	forcedTheme = value;
	isThemeForced = true;

	UpdateBrush();
}

bool TenMicaBackdrop::IsThemeForced::get() { return isThemeForced; }

void TenMicaBackdrop::IsThemeForced::set(bool value)
{
	isThemeForced = value;
	UpdateBrush();
}

bool TenMicaBackdrop::EnabledInActivatedNotForeground::get() { return enableInActivatedNotForeground; }

void TenMicaBackdrop::EnabledInActivatedNotForeground::set(bool value)
{
	enableInActivatedNotForeground = value;
	UpdateBrush();
}