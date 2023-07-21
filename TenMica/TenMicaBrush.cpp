#include "pch.h"
#include "CppHelpers.h"
#include "TenMicaBrush.h"
#include "WindowsRuntimeHelpers.h"
using namespace Windows::ApplicationModel;
using namespace Windows::System::Power;

using namespace TenMica;
using namespace Platform;

TenMicaBrush::TenMicaBrush() { Init(); }

TenMicaBrush::TenMicaBrush(ApplicationTheme ForcedTheme) : forcedTheme(ForcedTheme), isThemeForced(true) { Init(); }

void TenMicaBrush::Init()
{
	auto dwmapiLib = LoadLibrary("dwmapi.dll");

	lDwmpQueryThumbnailType = (DwmpQueryThumbnailType)GetProcAddress(dwmapiLib, MAKEINTRESOURCEA(114));
	lDwmpCreateSharedThumbnailVisual = (DwmpCreateSharedThumbnailVisual)GetProcAddress(dwmapiLib, MAKEINTRESOURCEA(147));
	lDwmpQueryWindowThumbnailSourceSize = (DwmpQueryWindowThumbnailSourceSize)GetProcAddress(dwmapiLib, MAKEINTRESOURCEA(162));
	lDwmUnregisterThumbnail = (DwmUnregisterThumbnail)GetProcAddress(dwmapiLib, "DwmUnregisterThumbnail");

	GetWindowRect = (GetWindowRectProto)GetProcAddress("user32.dll", "GetWindowRect");
	FindWindow = (FindWindowWProto)GetProcAddress("user32.dll", "FindWindowW");
	GetParent = (GetParentProto)GetProcAddress("user32.dll", "GetParent");
	//lSetWindowCompositionAttribute = (SetWindowCompositionAttribute)GetProcAddress("user32.dll", "SetWindowCompositionAttribute");

	legacyMode = !Windows::Foundation::Metadata::ApiInformation::IsMethodPresent(Compositor::typeid->FullName, "CreateVisualSurface");

	fallbackToSystemMica =
		Windows::Foundation::Metadata::ApiInformation::IsMethodPresent(Compositor::typeid->FullName, "TryCreateBlurredWallpaperBackdropBrush")
		&& VectorViewHasValue<Package^>(Package::Current->Dependencies, [](Package^ pkg) -> bool { return StringStartsWith(pkg->Id->Name->Data(), L"Microsoft.UI.Xaml.2") && (pkg->Id->Version.Major > 2 || pkg->Id->Version.Minor >= 60000); });
}

::CompositionBrush^ TenMicaBrush::BuildMicaEffectBrush(Compositor^ compositor, Visual^ src, Color tintColor, float tintOpacity, float luminosityOpacity, SIZE size)
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
	luminosityBlendEffect->Background = ref new CompositionEffectSourceParameter("BlurredWallpaperBackdrop");
	luminosityBlendEffect->Foreground = luminosityOpacityEffect;

	// Apply Tint:

	// Color Blend.
	var colorBlendEffect = ref new BlendEffect();
	colorBlendEffect->Mode = BlendEffectMode::Luminosity;
	colorBlendEffect->Background = luminosityBlendEffect;
	colorBlendEffect->Foreground = tintOpacityEffect;

	CompositionEffectBrush^ micaEffectBrush = compositor->CreateEffectFactory(colorBlendEffect)->CreateBrush();

	var srcSize = Windows::Foundation::Numerics::float2::float2(size.cx, size.cy);

	if (surface != nullptr)
	{
		try
		{
			Visual^ visual;
			visual = surface->SourceVisual;

			surface->SourceVisual = nullptr;
			delete surface;
			surface = nullptr;

			if (visual)
			{
				// We need an additional Release
				reinterpret_cast<IUnknown*>(visual)->Release();
				visual = nullptr;
			}
		} catch (...) { }
	}

	surface = compositor->CreateVisualSurface();
	surface->SourceVisual = src;
	surface->SourceSize = srcSize;

	var brushRaw = compositor->CreateSurfaceBrush(surface);
	brushRaw->Stretch = CompositionStretch::None;

	// Blur
	var blurEffect = ref new GaussianBlurEffect();
	blurEffect->Name = "Blur";
	blurEffect->BlurAmount = 140.0f;
	blurEffect->Source = ref new CompositionEffectSourceParameter("src");

	var brush = compositor->CreateEffectFactory(blurEffect)->CreateBrush();
	brush->SetSourceParameter("src", brushRaw);

	micaEffectBrush->SetSourceParameter("BlurredWallpaperBackdrop", brush);

	return micaEffectBrush;
}

::CompositionBrush^ TenMicaBrush::BuildMicaEffectBrushLegacy(Compositor^ compositor, IDCompositionVisual2* src, Color tintColor, float tintOpacity, float luminosityOpacity, SIZE size)
{
	ComPtr<IDCompositionDevice3> device;
	//ComPtr<IDCompositionDesktopDeviceRestricted> deviceRestricted; // For Win11 testing
	
	ComPtr<ABI::Windows::UI::Composition::IVisual> visualRaw;
	src->QueryInterface(visualRaw.GetAddressOf());
	auto vis = reinterpret_cast<Visual^>(visualRaw.Get());

	(reinterpret_cast<IInspectable*>(compositor))->QueryInterface(device.GetAddressOf());
	//device.As(&deviceRestricted); // For Win11 testing
	auto functions = GetFloodEffectAndVisualSurfaceFunctions(device.Get());

	auto floodThisPtr = std::get<0>(functions);
	auto surfaceThisPtr = std::get<1>(functions);
	auto CreateFloodEffect = std::get<CreateFloodEffectFunc>(functions);
	auto CreateVisualSurface = std::get<CreateVisualSurfaceFunc>(functions);

	if (!CreateFloodEffect || !CreateVisualSurface || !floodThisPtr || !surfaceThisPtr)
	{
		auto funcName = (!CreateFloodEffect || !floodThisPtr) && CreateVisualSurface && surfaceThisPtr ? "CreateFloodEffect" : ((!CreateFloodEffect || !floodThisPtr) ? "CreateFloodEffect and CreateVisualSurface" : "CreateVisualSurface");
		OutputFormattedString("[10Mica] Failed to determine memory address of %s function(s)\r\n", funcName);

		return nullptr;
	}

	// Tint Color.

	ComPtr<IDCompositionFloodEffect> tintOpacityEffect;
	D2D1_VECTOR_4F tintColorLuminosity = { tintColor.R / 255.0f, tintColor.G / 255.0f, tintColor.B / 255.0f, luminosityOpacity };
	D2D1_VECTOR_4F tintColorOpacity = { tintColor.R / 255.0f, tintColor.G / 255.0f, tintColor.B / 255.0f, tintOpacity };

	//deviceRestricted->CreateFloodEffect(tintOpacityEffect.GetAddressOf()); // For Win11 testing
	CreateFloodEffect(floodThisPtr, tintOpacityEffect.GetAddressOf());
	tintOpacityEffect->SetColor(tintColorOpacity);

	// Apply Luminosity:

	// Luminosity Color.
	ComPtr<IDCompositionFloodEffect> luminosityOpacityEffect;

	//deviceRestricted->CreateFloodEffect(luminosityOpacityEffect.GetAddressOf()); // For Win11 testing
	CreateFloodEffect(floodThisPtr, luminosityOpacityEffect.GetAddressOf());
	luminosityOpacityEffect->SetColor(tintColorLuminosity);

	ComPtr<IDCompositionGaussianBlurEffect> blurEffect;
	device->CreateGaussianBlurEffect(blurEffect.GetAddressOf());
	blurEffect->SetStandardDeviation(140.0f);

	// Luminosity Blend.
	ComPtr<IDCompositionBlendEffect> luminosityBlendEffect;
	device->CreateBlendEffect(luminosityBlendEffect.GetAddressOf());
	luminosityBlendEffect->SetMode(D2D1_BLEND_MODE_LUMINOSITY);
	luminosityBlendEffect->SetInput(0, blurEffect.Get(), 0);
	luminosityBlendEffect->SetInput(1, luminosityOpacityEffect.Get(), 0);

	// Apply Tint:

	// Color Blend.
	ComPtr<IDCompositionBlendEffect> colorBlendEffect;
	device->CreateBlendEffect(colorBlendEffect.GetAddressOf());
	colorBlendEffect->SetMode(D2D1_BLEND_MODE_COLOR);
	colorBlendEffect->SetInput(0, luminosityBlendEffect.Get(), 0);
	colorBlendEffect->SetInput(1, tintOpacityEffect.Get(), 0);

	HRESULT hr = src->SetEffect(colorBlendEffect.Get());

	var srcSize = Windows::Foundation::Numerics::float2::float2(size.cx, size.cy);

	if (surfaceLegacy != nullptr)
	{
		try
		{
			surfaceLegacy->Source = nullptr;
			delete surfaceLegacy;
			surfaceLegacy = nullptr;
		}
		catch (...) {}
	}

	surfaceLegacy = GetVisualSurface(surfaceThisPtr, CreateVisualSurface);
	surfaceLegacy->Source = vis;
	surfaceLegacy->SourceRectangleRight = size.cx;
	surfaceLegacy->SourceRectangleBottom = size.cy;

	var brushRaw = compositor->CreateSurfaceBrush((ICompositionSurface^)surfaceLegacy);
	brushRaw->Stretch = CompositionStretch::None;

	((IUnknown*)floodThisPtr)->Release();
	((IUnknown*)surfaceThisPtr)->Release();

	return brushRaw;
}

// Ugly hack, basically
void TenMicaBrush::UpdateVisual(RECT rect)
{
	if (!legacyMode)
	{
		surface->SourceSize = Windows::Foundation::Numerics::float2::float2(rect.right - rect.left, rect.bottom - rect.top);
		surface->SourceOffset = Windows::Foundation::Numerics::float2::float2(rect.left, rect.top);
	}
	else
	{
		surfaceLegacy->SourceRectangleBottom = rect.bottom;
		surfaceLegacy->SourceRectangleLeft = rect.left;
		surfaceLegacy->SourceRectangleRight = rect.right;
		surfaceLegacy->SourceRectangleTop = rect.top;
	}
}

void TenMicaBrush::OnConnected()
{
	if (CompositionBrush != nullptr) return;
	if (DesignMode::DesignModeEnabled) return;
	if (Windows::System::Profile::AnalyticsInfo::VersionInfo->DeviceFamily != "Windows.Desktop") return;
	if (!Enabled) return;
	if (FallbackToSystemMica && Windows::Foundation::Metadata::ApiInformation::IsMethodPresent(Compositor::typeid->FullName, "TryCreateBlurredWallpaperBackdropBrush")) return;

	cWindow = CoreWindow::GetForCurrentThread(); // Assuming we use CoreWindow
	CoreWindow^ coreWnd = cWindow;

	if (uiSettings == null)
	{
		uiSettings = ref new UISettings();
	}

	if (accessibilitySettings == null)
	{
		accessibilitySettings = ref new AccessibilitySettings();
	}

	fastEffects = CompositionCapabilities::GetForCurrentView()->AreEffectsFast();

	energySaver = PowerManager::EnergySaverStatus == EnergySaverStatus::On;

	windowActivated = legacyMode ? true : coreWnd->ActivationMode == CoreWindowActivationMode::ActivatedInForeground
		|| (enableInActivatedNotForeground && coreWnd->ActivationMode == CoreWindowActivationMode::ActivatedNotForeground);

	ComPtr<ABI::Windows::UI::Core::ICoreWindow> coreWndRaw;
	coreWndRaw = reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(coreWnd);

	// Getting our HWND
	ComPtr<ICoreWindowInterop> interop;
	coreWndRaw.As<ICoreWindowInterop>(&interop);
	interop->get_WindowHandle(&cHwnd);

	UpdateBrush();

	OnActivatedCookie = coreWnd->Activated += ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::WindowActivatedEventArgs^>(this, &TenMica::TenMicaBrush::OnActivated);

	OnColorValuesChangedCookie = uiSettings->ColorValuesChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::ViewManagement::UISettings^, Platform::Object^>(this, &TenMica::TenMicaBrush::OnColorValuesChanged);
	OnHighContrastChangedCookie = accessibilitySettings->HighContrastChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::ViewManagement::AccessibilitySettings^, Platform::Object^>(this, &TenMica::TenMicaBrush::OnHighContrastChanged);
	OnCompositionCapabilitiesChangedCookie = CompositionCapabilities::GetForCurrentView()->Changed += ref new Windows::Foundation::TypedEventHandler<Windows::UI::Composition::CompositionCapabilities^, Platform::Object^>(this, &TenMica::TenMicaBrush::OnCompositionCapabilitiesChanged);
	OnEnergySaverStatusChangedCookie = PowerManager::EnergySaverStatusChanged += ref new Windows::Foundation::EventHandler<Platform::Object^>(this, &TenMica::TenMicaBrush::OnEnergySaverStatusChanged);

	// Syncing!!
	IInternalCoreWindow2^ internalW2 = dynamic_cast<IInternalCoreWindow2^>(coreWnd); // Pure black magic
	if (internalW2)
		OnWindowPositionChangedCookie = internalW2->WindowPositionChanged += ref new TypedEventHandler<IInternalCoreWindow2^, Platform::Object^>(this, &TenMicaBrush::OnWindowPositionChanged);

	IInternalCoreWindow^ internalW1 = dynamic_cast<IInternalCoreWindow^>(coreWnd); // Pure blue magic
	if (internalW1)
		OnDisplayChangedCookie = internalW1->DisplayChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Platform::Object^>(this, &TenMica::TenMicaBrush::OnDisplayChanged);
}

void TenMicaBrush::OnDisplayChanged(Windows::UI::Core::CoreWindow^ sender, Platform::Object^ args)
{
	sender->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this, sender]()
	{
		UpdateBrush();
	}));
}

void TenMicaBrush::OnWindowPositionChanged(IInternalCoreWindow2^ window, Platform::Object^ args)
{
	if (surface != nullptr || surfaceLegacy != nullptr)
	{
		RECT rect;
		GetWindowRect(cHwnd, &rect);
		UpdateVisual(rect);
	}
}

void TenMicaBrush::OnColorValuesChanged(UISettings^ sender, Object^ args)
{
	cWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this, sender]()
	{
		UpdateBrush();
	}));
}

void TenMicaBrush::OnHighContrastChanged(AccessibilitySettings^ sender, Platform::Object^ args)
{
	cWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this, sender]()
	{
		UpdateBrush();
	}));
}

void TenMicaBrush::OnEnergySaverStatusChanged(Platform::Object^ sender, Platform::Object^ e)
{
	cWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this, sender]()
	{
		energySaver = PowerManager::EnergySaverStatus == EnergySaverStatus::On;
		UpdateBrush();
	}));
}

void TenMicaBrush::OnCompositionCapabilitiesChanged(CompositionCapabilities^ sender, Platform::Object^ args)
{
	cWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this, sender]()
	{
		fastEffects = CompositionCapabilities::GetForCurrentView()->AreEffectsFast();
		UpdateBrush();
	}));
}

void TenMicaBrush::OnActivated(CoreWindow^ sender, WindowActivatedEventArgs^ args)
{
	sender->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this, sender, args]()
	{
		var activated = legacyMode ? (args->WindowActivationState != CoreWindowActivationState::Deactivated || (enableInActivatedNotForeground && sender->Visible)) : (sender->ActivationMode == CoreWindowActivationMode::ActivatedInForeground
			|| (enableInActivatedNotForeground && (sender->ActivationMode == CoreWindowActivationMode::ActivatedNotForeground || sender->Visible)));
		if (windowActivated && activated == windowActivated)
		{
			return;
		}
		windowActivated = activated;
		UpdateBrush();
	}));
}

::CompositionBrush^ TenMicaBrush::CreateCrossFadeEffectBrush(Compositor^ compositor, ::CompositionBrush^ from, ::CompositionBrush^ to)
{
	var crossFadeEffect = ref new CrossFadeEffect();
	crossFadeEffect->Name = "Crossfade";
	crossFadeEffect->Source1 = ref new CompositionEffectSourceParameter("source1");
	crossFadeEffect->Source2 = ref new CompositionEffectSourceParameter("source2");
	crossFadeEffect->CrossFade = 0;

	auto list = ref new Platform::Collections::Vector<String^>();
	list->Append("Crossfade.CrossFade");

	CompositionEffectBrush^ crossFadeEffectBrush = compositor->CreateEffectFactory(crossFadeEffect, list)->CreateBrush();
	crossFadeEffectBrush->Comment = "Crossfade";
	crossFadeEffectBrush->SetSourceParameter("source1", to);
	crossFadeEffectBrush->SetSourceParameter("source2", from);
	return crossFadeEffectBrush;
}

ScalarKeyFrameAnimation^ TenMicaBrush::CreateCrossFadeAnimation(Compositor^ compositor)
{
	ScalarKeyFrameAnimation^ animation = compositor->CreateScalarKeyFrameAnimation();
	LinearEasingFunction^ linearEasing = compositor->CreateLinearEasingFunction();
	animation->InsertKeyFrame(0.0f, 0.0f, linearEasing);
	animation->InsertKeyFrame(1.0f, 1.0f, linearEasing);
	animation->Duration = TimeSpan{ 2500000 };
	return animation;
}

void TenMicaBrush::UpdateBrush()
{
	if (uiSettings == nullptr || accessibilitySettings == nullptr)
		return;

	if (FallbackToSystemMica)
	{
		OnDisconnected();
		FallbackColor = Colors::Transparent;

		return;
	}

	bool useSolidColorFallback = !windowActivated || fastEffects == false || energySaver == true || !uiSettings->AdvancedEffectsEnabled;

	var compositor = Window::Current->Compositor;

	var isLightMode = ThemeForced ? ForcedTheme == ApplicationTheme::Light : Application::Current->RequestedTheme == ApplicationTheme::Light;
	Color tintColor = isLightMode ? Color{ 255, 243, 243, 243 } : Color{ 255, 32, 32, 32 };
	float tintOpacity = isLightMode ? 0.5f : 0.8f;

	if (accessibilitySettings->HighContrast)
	{
		tintColor = uiSettings->GetColorValue(UIColorType::Background);
		useSolidColorFallback = true;
	}

	FallbackColor = tintColor;

	::CompositionBrush^ newBrush;

	if (useSolidColorFallback)
		newBrush = compositor->CreateColorBrush(tintColor);
	else
	{
		CoreWindow^ coreWnd = cWindow; // Assuming we use CoreWindow
		HWND targetWindow = (HWND)FindWindow(L"Progman", NULL); // Progman is still the name of the Desktop window :))))

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

		if (hThumbWindow && !legacyMode)
		{
			lDwmUnregisterThumbnail(hThumbWindow);
			hThumbWindow = NULL;
		}

		ComPtr<IDCompositionDevice3> dcompDevice;
		(reinterpret_cast<IInspectable*>(compositor))->QueryInterface(dcompDevice.GetAddressOf());

		ComPtr<IDCompositionVisual2> windowVisual;

		if (!legacyMode || windowVisualLegacy == nullptr)
		{
			HRESULT result = lDwmpCreateSharedThumbnailVisual(legacyMode ? cHwnd : GetParent(cHwnd), targetWindow, 2, &thumb, dcompDevice.Get(), (void**)windowVisual.GetAddressOf(), &hThumbWindow);
			if (legacyMode && result == E_INVALIDARG) result = lDwmpCreateSharedThumbnailVisual(GetParent(cHwnd), targetWindow, 2, &thumb, dcompDevice.Get(), (void**)windowVisual.GetAddressOf(), &hThumbWindow);
			if (result != S_OK)
			{
				OutputFormattedString("[10Mica] The call to DwmpCreateSharedThumbnailVisual failed with 0x%X, this often happens when the brush is created before the window is activated or shown.\r\n", result);
				return;
			}

			windowVisualLegacy = windowVisual;
		}
		else
			windowVisual = windowVisualLegacy;

		if (!legacyMode)
		{
			ComPtr<ABI::Windows::UI::Composition::IVisual> visualRaw;
			windowVisual.As<ABI::Windows::UI::Composition::IVisual>(&visualRaw);
			auto visual = reinterpret_cast<Visual^>(visualRaw.Get());

			newBrush = BuildMicaEffectBrush(compositor, visual, tintColor, tintOpacity, 1.0f, windowSize);
		}
		else
			newBrush = BuildMicaEffectBrushLegacy(compositor, windowVisual.Get(), tintColor, tintOpacity, 1.0f, windowSize);

		RECT rect;
		GetWindowRect(cHwnd, &rect);
		UpdateVisual(rect);
	}

	::CompositionBrush^ oldBrush = CompositionBrush;

	if (oldBrush == nullptr || (CompositionBrush->Comment == "Crossfade") || (dynamic_cast<CompositionColorBrush^>(oldBrush) != nullptr && dynamic_cast<CompositionColorBrush^>(newBrush) != nullptr))
	{
		// Set new brush directly
		if (oldBrush != nullptr)
			delete oldBrush;

		CompositionBrush = newBrush;
	}
	else if (oldBrush && newBrush)
	{
		// Crossfade
		::CompositionBrush^ crossFadeBrush = CreateCrossFadeEffectBrush(compositor, oldBrush, newBrush);
		ScalarKeyFrameAnimation^ animation = CreateCrossFadeAnimation(compositor);
		CompositionBrush = crossFadeBrush;

		var crossFadeAnimationBatch = compositor->CreateScopedBatch(CompositionBatchTypes::Animation);
		crossFadeBrush->StartAnimation("CrossFade.CrossFade", animation);
		crossFadeAnimationBatch->End();

		crossFadeAnimationBatch->Completed += ref new TypedEventHandler<Platform::Object^, CompositionBatchCompletedEventArgs^>([this, crossFadeBrush, oldBrush, newBrush](Platform::Object^, CompositionBatchCompletedEventArgs^)
		{
			delete crossFadeBrush;
			delete oldBrush;
			this->CompositionBrush = newBrush;
		});
	}
}

void TenMicaBrush::OnDisconnected()
{
	if (CompositionBrush != nullptr)
	{
		try
		{
			delete CompositionBrush;
			CompositionBrush = null;

			if (cWindow != null)
			{
				cWindow->Activated -= OnActivatedCookie;
				IInternalCoreWindow2^ internalW2 = dynamic_cast<IInternalCoreWindow2^>(cWindow);
				if (internalW2)
					internalW2->WindowPositionChanged -= OnWindowPositionChangedCookie;

				IInternalCoreWindow^ internalW1 = dynamic_cast<IInternalCoreWindow^>(cWindow);
				if (internalW1)
					internalW1->DisplayChanged -= OnDisplayChangedCookie;

				cWindow = null;
			}

			if (uiSettings != null)
			{
				uiSettings->ColorValuesChanged -= OnColorValuesChangedCookie;
				uiSettings = null;
			}

			if (accessibilitySettings != null)
			{
				accessibilitySettings->HighContrastChanged -= OnHighContrastChangedCookie;
				accessibilitySettings = null;
			}

			CompositionCapabilities::GetForCurrentView()->Changed -= OnCompositionCapabilitiesChangedCookie;
			PowerManager::EnergySaverStatusChanged -= OnEnergySaverStatusChangedCookie;

			if (!legacyMode)
			{
				Visual^ visual = surface->SourceVisual;
				surface->SourceVisual = null;
				delete surface;
				surface = null;

				if (visual)
				{
					// We need an additional Release
					reinterpret_cast<IUnknown*>(visual)->Release();
					visual = null;
				}
			}
			else
			{
				surfaceLegacy->Source = null;
				delete surfaceLegacy;
				surfaceLegacy = null;
			}

			lDwmUnregisterThumbnail(hThumbWindow);
		} catch (...) { }
	}
}

ApplicationTheme TenMicaBrush::ForcedTheme::get() { return forcedTheme; }

void TenMicaBrush::ForcedTheme::set(ApplicationTheme value)
{
	forcedTheme = value;
	ThemeForced = true;

	UpdateBrush();
}

bool TenMicaBrush::ThemeForced::get() { return isThemeForced; }

void TenMicaBrush::ThemeForced::set(bool value)
{
	isThemeForced = value;
	UpdateBrush();
}

bool TenMicaBrush::Enabled::get() { return isEnabled; }

void TenMicaBrush::Enabled::set(bool value)
{
	isEnabled = value;
	
	if (!isEnabled)
	{
		OnDisconnected();
		FallbackColor = Colors::Transparent;
	}
	else
		OnConnected();
}

bool TenMicaBrush::FallbackToSystemMica::get() { return fallbackToSystemMica; }

void TenMicaBrush::FallbackToSystemMica::set(bool value) 
{
	fallbackToSystemMica = 
		value ? 
		(
			Windows::Foundation::Metadata::ApiInformation::IsMethodPresent(Compositor::typeid->FullName, "TryCreateBlurredWallpaperBackdropBrush") 
			&& VectorViewHasValue<Package^>(Package::Current->Dependencies, [](Package^ pkg) -> bool { return StringStartsWith(pkg->Id->Name->Data(), L"Microsoft.UI.Xaml.2") && (pkg->Id->Version.Major > 2 || pkg->Id->Version.Minor >= 60000); })
			? value : false
		) 
		: value;
}

bool TenMicaBrush::EnabledInActivatedNotForeground::get() { return enableInActivatedNotForeground; }

void TenMicaBrush::EnabledInActivatedNotForeground::set(bool value)
{
	enableInActivatedNotForeground = value;
	UpdateBrush();
}

void TenMicaBrush::OnElementConnected(DependencyObject^ element)
{
	if (FallbackToSystemMica && dynamic_cast<Control^>(element) != nullptr)
	{
		if (CompositionBrush)
			OnDisconnected();

		ApplyMicaToBackground(element);

		FallbackColor = Colors::Transparent;
	}
}