#include "pch.h"
#include "TenMicaBrush.h"
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Graphics::Capture;
using namespace Microsoft::Graphics::Canvas;
using namespace Microsoft::Graphics::Canvas::UI::Composition;
using namespace Windows::ApplicationModel;
using namespace Windows::System::Power;

using namespace TenMica;
using namespace Platform;

TenMicaBrush::TenMicaBrush() { Init(); }

TenMicaBrush::TenMicaBrush(ApplicationTheme ForcedTheme) : forcedTheme(ForcedTheme), isThemeForced(true) { Init(); }

void TenMicaBrush::Init()
{
	auto dwmapiLib = LoadLibrary(L"dwmapi.dll");
	auto user32 = GetModuleHandle(L"user32.dll");

	lDwmpQueryThumbnailType = (DwmpQueryThumbnailType)GetProcAddress(dwmapiLib, MAKEINTRESOURCEA(114));
	lDwmpCreateSharedThumbnailVisual = (DwmpCreateSharedThumbnailVisual)GetProcAddress(dwmapiLib, MAKEINTRESOURCEA(147));
	lDwmpQueryWindowThumbnailSourceSize = (DwmpQueryWindowThumbnailSourceSize)GetProcAddress(dwmapiLib, MAKEINTRESOURCEA(162));

	lSetWindowCompositionAttribute = (SetWindowCompositionAttribute)GetProcAddress(user32, "SetWindowCompositionAttribute");
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
	luminosityBlendEffect->Background = (Windows::Graphics::Effects::IGraphicsEffectSource^)ref new CompositionEffectSourceParameter("BlurredWallpaperBackdrop");
	luminosityBlendEffect->Foreground = luminosityOpacityEffect;

	// Apply Tint:

	// Color Blend.
	var colorBlendEffect = ref new BlendEffect();
	colorBlendEffect->Mode = BlendEffectMode::Luminosity;
	colorBlendEffect->Background = luminosityBlendEffect;
	colorBlendEffect->Foreground = tintOpacityEffect;

	CompositionEffectBrush^ micaEffectBrush = compositor->CreateEffectFactory(colorBlendEffect)->CreateBrush();

	var srcSize = Windows::Foundation::Numerics::float2::float2(size.cx, size.cy);

	/*if (surface != nullptr)
	{
		try
		{
			Visual^ visual;
			visual = surface->SourceVisual;

			surface->SourceVisual = nullptr;
			delete surface;
			surface = nullptr;
			//delete visual;
			//visual = nullptr;
		} catch (...) { }
	}*/

	src->Size = srcSize;

	surface = compositor->CreateVisualSurface();
	surface->SourceVisual = src;
	surface->SourceSize = srcSize;

	var brushRaw = compositor->CreateSurfaceBrush(surface);
	brushRaw->Stretch = CompositionStretch::None;


	// Blur
	var blurEffect = ref new GaussianBlurEffect();
	blurEffect->Name = "Blur";
	blurEffect->BlurAmount = 140.0f;
	blurEffect->Source = (Windows::Graphics::Effects::IGraphicsEffectSource^)ref new CompositionEffectSourceParameter("src");

	var brush = compositor->CreateEffectFactory(blurEffect)->CreateBrush();
	brush->SetSourceParameter("src", brushRaw);

	micaEffectBrush->SetSourceParameter("BlurredWallpaperBackdrop", brush);

	return micaEffectBrush;
}

// Ugly hack, basically
void TenMicaBrush::UpdateVisual(RECT rect)
{
	surface->SourceSize = Windows::Foundation::Numerics::float2::float2(rect.right - rect.left, rect.bottom - rect.top);
	surface->SourceOffset = Windows::Foundation::Numerics::float2::float2(rect.left, rect.top);
}

/*LRESULT CALLBACK HlprWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		//PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProcW(hwnd, message, wParam, lParam);
}*/

#define null nullptr

typedef HRESULT(STDAPICALLTYPE* PFNGETACTIVATIONFACTORY2)(HSTRING, void**);

void TenMicaBrush::OnConnected()
{
	if (CompositionBrush != nullptr) return;

	if (DesignMode::DesignModeEnabled) return;

	cWindow = TargetWindow;

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

	windowActivated = cWindow->Visible || enableInActivatedNotForeground;

	ComPtr<IWindowNative> coreWndRaw;
	((IUnknown*)cWindow)->QueryInterface(coreWndRaw.GetAddressOf());

	// Getting our HWND
	coreWndRaw->get_WindowHandle(&cHwnd);

	appWindow = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(Microsoft::UI::WindowId{ (unsigned long long)cHwnd });

	// TEMP HACK-y WORKAROUND

	/*HINSTANCE hInstance = GetModuleHandle3();

	WNDCLASS wc = {};
	wc.lpfnWndProc = HlprWndProc;
	wc.hInstance = hInstance;
	wc.cbWndExtra = 0;
	wc.lpszClassName = L"TenMicaHelperWindow";

	RegisterClassW(&wc);

	hwndHelper = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT, L"TenMicaHelperWindow", L"TenMicaHelperWindow",
		NULL, CW_USEDEFAULT, CW_USEDEFAULT, 1, 1,
		nullptr, nullptr, hInstance, NULL);

	BOOL enable = TRUE;
	WINDOWCOMPOSITIONATTRIBDATA wData{};
	wData.Attrib = WCA_CLOAK;
	wData.pvData = &enable;
	wData.cbData = sizeof(BOOL);
	lSetWindowCompositionAttribute(hwndHelper, &wData);

	ShowWindow(hwndHelper, SW_SHOWNA);*/

	CreateDevice();
	winComp = InitializeInteropCompositor(d2dDevice.Get());

	/*HMODULE dcompi = GetModuleHandle(L"dcompi.dll");
	PFNGETACTIVATIONFACTORY DcompGetActivationFactory = (PFNGETACTIVATIONFACTORY)GetProcAddress(dcompi, "DllGetActivationFactory");

	ComPtr<IActivationFactory> proxyStaticsRaw;
	ComPtr<ISystemVisualProxyVisualPrivateStaticsRaw> proxyStatics;
	HRESULT hr = DcompGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(L"Microsoft.UI.Composition.Private.SystemVisualProxyVisualPrivate").Get(), proxyStaticsRaw.GetAddressOf());
	hr = proxyStaticsRaw->QueryInterface(proxyStatics.GetAddressOf());

	ISystemVisualProxyVisualPrivate^ proxy;
	auto comp = TargetWindow->Compositor;
	hr = proxyStatics->Create(comp, &proxy);
	uVisual = (Visual^)proxy;
	uVisual->Size = Windows::Foundation::Numerics::float2::float2(100, 100);

	void* handle;
	ComPtr<ISystemVisualProxyVisualPrivateInterop> proxyInterop;
	((IUnknown*)proxy)->QueryInterface(proxyInterop.GetAddressOf());
	proxyInterop->GetHandle(&handle);

	ComPtr<ICompositorPartner> partner;
	((IUnknown*)winComp)->QueryInterface(partner.GetAddressOf());
	hr = partner->OpenSharedTargetFromHandle(handle, cTarget.GetAddressOf());*/

	auto comp = TargetWindow->Compositor;
	auto d3dd = CreateDirect3DDevice(dxgiDevice.Get());
	auto canvasD = CanvasDevice::CreateFromDirect3D11Device(d3dd);
	theGraphicsDevice = CanvasComposition::CreateCompositionGraphicsDevice(comp, canvasD);
	theBrush = comp->CreateSurfaceBrush();
	theBrush->Stretch = CompositionStretch::None;
	spriteVisual = comp->CreateSpriteVisual();
	spriteVisual->Brush = theBrush;

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

	HTHUMBNAIL hThumbWindow;
	ComPtr<IDCompositionVisual2> windowVisual;

	//TODO: check if comp implements IDCompositionDevice before calling, it doesn't implement it if we aren't running under XAML or having no InteropCompositor
	auto result = lDwmpCreateSharedThumbnailVisual(cHwnd, targetWindow, 2, &thumb, (IDCompositionDevice*)winComp, (void**)windowVisual.GetAddressOf(), &hThumbWindow);

	windowVisual->SetOffsetX(0.0f);
	windowVisual->SetOffsetY(0.0f);

	// you can cast directly but this is safer, like in case we don't have an InteropCompositor
	ComPtr<ABI::Windows::UI::Composition::IVisual> visualRaw;
	windowVisual->QueryInterface(visualRaw.GetAddressOf());
	//TODO: check if visualRaw isn't nullptr before casting
	auto visual = reinterpret_cast<Windows::UI::Composition::Visual^>(visualRaw.Get());
	//visual->Size = Windows::Foundation::Numerics::float2::float2(windowSize.cx, windowSize.cy);
	auto refSize = Windows::Foundation::Numerics::float2::float2(windowSize.cx, windowSize.cy);
	spriteVisual->Size = refSize;
	//visual->Offset = Windows::Foundation::Numerics::float3::float3(0, 0, 0);

	//ComPtr<ABI::Windows::UI::Composition::ICompositionTarget> target;
	//cTarget->QueryInterface(target.GetAddressOf());

	//target->put_Root(visualRaw.Get());
	auto drawingSurface = theGraphicsDevice->CreateDrawingSurface(Size{ (float)windowSize.cx, (float)windowSize.cy }, Microsoft::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, Microsoft::Graphics::DirectX::DirectXAlphaMode::Premultiplied);

	theBrush->Surface = drawingSurface;

	auto item = GraphicsCaptureItem::CreateFromVisual(visual);
	pool = Direct3D11CaptureFramePool::CreateFreeThreaded(d3dd, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, SizeInt32{ windowSize.cx, windowSize.cy });
	gS = pool->CreateCaptureSession(item);
	FrameArrivedCookie = pool->FrameArrived += ref new TypedEventHandler<Direct3D11CaptureFramePool^, Object^>([drawingSurface, canvasD](Direct3D11CaptureFramePool^ sender, Object^ args)
	{
		auto frame = sender->TryGetNextFrame();
		auto drawingS = CanvasComposition::CreateDrawingSession(drawingSurface);
		auto bitmap = CanvasBitmap::CreateFromDirect3D11Surface(drawingS, frame->Surface);
		drawingS->DrawImage(bitmap);

		delete frame;
		delete bitmap;
		delete drawingS;
	});

	winComp->RequestCommitAsync();
	comp->RequestCommitAsync();

	gS->StartCapture();

	UpdateBrush();

	OnActivatedCookie = cWindow->Activated += ref new Windows::Foundation::TypedEventHandler<Object^, Microsoft::UI::Xaml::WindowActivatedEventArgs^>(this, &TenMica::TenMicaBrush::OnActivated);

	OnColorValuesChangedCookie = uiSettings->ColorValuesChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::ViewManagement::UISettings^, Platform::Object^>(this, &TenMica::TenMicaBrush::OnColorValuesChanged);
	//OnHighContrastChangedCookie = accessibilitySettings->HighContrastChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::ViewManagement::AccessibilitySettings^, Platform::Object^>(this, &TenMica::TenMicaBrush::OnHighContrastChanged);
	OnCompositionCapabilitiesChangedCookie = compCapabilities->Changed += ref new Windows::Foundation::TypedEventHandler<Microsoft::UI::Composition::CompositionCapabilities^, Platform::Object^>(this, &TenMica::TenMicaBrush::OnCompositionCapabilitiesChanged);
	OnEnergySaverStatusChangedCookie = PowerManager::EnergySaverStatusChanged += ref new Windows::Foundation::EventHandler<Platform::Object^>(this, &TenMica::TenMicaBrush::OnEnergySaverStatusChanged);

	//IInternalCoreWindow2^ internalW = (IInternalCoreWindow2^)coreWnd; // Pure black magic

	// Syncing!!
	//OnWindowPositionChangedCookie = internalW->WindowPositionChanged += ref new TypedEventHandler<IInternalCoreWindow2^, Platform::Object^>(this, &TenMicaBrush::OnWindowPositionChanged);

	OnWindowPositionChangedCookie = appWindow->Changed += ref new TypedEventHandler<Microsoft::UI::Windowing::AppWindow^, AppWindowChangedEventArgs^>(this, &TenMica::TenMicaBrush::OnAppWindowChanged);

	//IInternalCoreWindow^ internalW1 = (IInternalCoreWindow^)coreWnd; // Pure blue magic
	//OnDisplayChangedCookie = internalW1->DisplayChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Platform::Object^>(this, &TenMica::TenMicaBrush::OnDisplayChanged);
}

bool TenMicaBrush::CreateDevice()
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

Windows::UI::Composition::Compositor^ TenMicaBrush::InitializeInteropCompositor(IUnknown* d2dDevice)
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

void TenMicaBrush::OnAppWindowChanged(Microsoft::UI::Windowing::AppWindow^ sender, AppWindowChangedEventArgs^ args)
{
	if (surface != nullptr && args->DidPositionChange)
	{
		RECT rect;
		GetWindowRect(cHwnd, &rect);
		UpdateVisual(rect);
	}
}

void TenMicaBrush::OnDisplayChanged(Windows::UI::Core::CoreWindow^ sender, Platform::Object^ args)
{
	cWindow->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this, sender]()
	{
		UpdateBrush();
	}));
}

void TenMicaBrush::OnWindowPositionChanged(IInternalCoreWindow2^ window, Platform::Object^ args)
{
	if (surface != nullptr)
	{
		RECT rect;
		GetWindowRect(cHwnd, &rect);
		UpdateVisual(rect);
	}
}

void TenMicaBrush::OnColorValuesChanged(UISettings^ sender, Object^ args)
{
	cWindow->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this, sender]()
	{
		UpdateBrush();
	}));
}

void TenMicaBrush::OnHighContrastChanged(AccessibilitySettings^ sender, Platform::Object^ args)
{
	cWindow->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this, sender]()
	{
		UpdateBrush();
	}));
}

void TenMicaBrush::OnEnergySaverStatusChanged(Platform::Object^ sender, Platform::Object^ e)
{
	cWindow->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this, sender]()
	{
		energySaver = PowerManager::EnergySaverStatus == EnergySaverStatus::On;
		UpdateBrush();
	}));
}

void TenMicaBrush::OnCompositionCapabilitiesChanged(CompositionCapabilities^ sender, Platform::Object^ args)
{
	cWindow->DispatcherQueue->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([this, sender]()
	{
		fastEffects = compCapabilities->AreEffectsFast();
		UpdateBrush();
	}));
}

void TenMicaBrush::OnActivated(Object^ sender, Microsoft::UI::Xaml::WindowActivatedEventArgs^ args)
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

::CompositionBrush^ TenMicaBrush::CreateCrossFadeEffectBrush(Compositor^ compositor, ::CompositionBrush^ from, ::CompositionBrush^ to)
{
	var crossFadeEffect = ref new CrossFadeEffect();
	crossFadeEffect->Name = "Crossfade"; // Name to reference when starting the animation.
	crossFadeEffect->Source1 = (Windows::Graphics::Effects::IGraphicsEffectSource^)ref new CompositionEffectSourceParameter("source1");
	crossFadeEffect->Source2 = (Windows::Graphics::Effects::IGraphicsEffectSource^)ref new CompositionEffectSourceParameter("source2");
	crossFadeEffect->CrossFade = 0;

	auto list = ref new Platform::Collections::Vector<String^>();
	list->Append("Crossfade.CrossFade");

	CompositionEffectBrush^ crossFadeEffectBrush = compositor->CreateEffectFactory(crossFadeEffect, list)->CreateBrush();
	crossFadeEffectBrush->Comment = "Crossfade";
	// The inputs have to be swapped here to work correctly...
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
	{
		return;
	}

	bool useSolidColorFallback = !windowActivated || fastEffects == false || energySaver == true || !uiSettings->AdvancedEffectsEnabled;

	var compositor = TargetWindow->Compositor;

	var isLightMode = isThemeForced ? ForcedTheme == ApplicationTheme::Light : Application::Current->RequestedTheme == ApplicationTheme::Light;
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

		newBrush = BuildMicaEffectBrush(compositor, spriteVisual, tintColor, tintOpacity, 1.0f, windowSize);

		RECT rect;
		GetWindowRect(cHwnd, &rect);
		UpdateVisual(rect);

		winComp->RequestCommitAsync();
		compositor->RequestCommitAsync();
	}

	::CompositionBrush^ oldBrush = CompositionBrush;

	if (oldBrush == nullptr || (CompositionBrush->Comment == "Crossfade") || (dynamic_cast<CompositionColorBrush^>(oldBrush) != nullptr && dynamic_cast<CompositionColorBrush^>(newBrush) != nullptr))
	{
		// Set new brush directly
		if (oldBrush != nullptr)
		{
			delete oldBrush;
		}
		CompositionBrush = newBrush;
	}
	else
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
	//TODO: do proper disposing
	if (CompositionBrush != nullptr)
	{
		try
		{
			delete CompositionBrush;
			CompositionBrush = null;

			if (cWindow != null)
			{
				cWindow->Activated -= OnActivatedCookie;
				appWindow->Changed -= OnWindowPositionChangedCookie;

				cWindow = null;
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

			compCapabilities->Changed -= OnCompositionCapabilitiesChangedCookie;
			compCapabilities = null;

			PowerManager::EnergySaverStatusChanged -= OnEnergySaverStatusChangedCookie;

			pool->FrameArrived -= FrameArrivedCookie;
			delete gS;
			gS = null;
			delete pool;
			pool = null;

			Visual^ visual;
			visual = surface->SourceVisual;

			surface->SourceVisual = null;
			delete surface;
			surface = null;

			delete spriteVisual;
			spriteVisual = null;

			delete theGraphicsDevice;
			theGraphicsDevice = null;

			delete theBrush;
			theBrush = null;

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

ApplicationTheme TenMicaBrush::ForcedTheme::get() { return forcedTheme; }

void TenMicaBrush::ForcedTheme::set(ApplicationTheme value)
{
	forcedTheme = value;
	isThemeForced = true;

	UpdateBrush();
}

bool TenMicaBrush::IsThemeForced::get() { return isThemeForced; }

void TenMicaBrush::IsThemeForced::set(bool value)
{
	isThemeForced = value;
	UpdateBrush();
}

bool TenMicaBrush::EnabledInActivatedNotForeground::get() { return enableInActivatedNotForeground; }

void TenMicaBrush::EnabledInActivatedNotForeground::set(bool value)
{
	enableInActivatedNotForeground = value;
	UpdateBrush();
}