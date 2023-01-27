#pragma once

#include <Windows.h>
#include <dwmapi.h>
#include <dcomp.h>

enum THUMBNAIL_TYPE
{
	TT_DEFAULT = 0x0,
	TT_SNAPSHOT = 0x1,
	TT_ICONIC = 0x2,
	TT_BITMAPPENDING = 0x3,
	TT_BITMAP = 0x4
};

typedef HRESULT(WINAPI* DwmpCreateSharedThumbnailVisual)(
	IN HWND hwndDestination,
	IN HWND hwndSource,
	IN DWORD dwThumbnailFlags,
	IN DWM_THUMBNAIL_PROPERTIES* pThumbnailProperties,
	IN VOID* pDCompDevice,
	OUT VOID** ppVisual,
	OUT PHTHUMBNAIL phThumbnailId);

typedef HRESULT(WINAPI* DwmpQueryWindowThumbnailSourceSize)(
	IN HWND hwndSource,
	IN BOOL fSourceClientAreaOnly,
	OUT SIZE* pSize);

typedef HRESULT(WINAPI* DwmpQueryThumbnailType)(
	IN HTHUMBNAIL hThumbnailId,
	OUT THUMBNAIL_TYPE* thumbType);

#define DWM_TNP_FREEZE            0x100000
#define DWM_TNP_ENABLE3D          0x4000000
#define DWM_TNP_DISABLE3D         0x8000000
#define DWM_TNP_FORCECVI          0x40000000
#define DWM_TNP_DISABLEFORCECVI   0x80000000

enum WINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0x0,
	WCA_NCRENDERING_ENABLED = 0x1,
	WCA_NCRENDERING_POLICY = 0x2,
	WCA_TRANSITIONS_FORCEDISABLED = 0x3,
	WCA_ALLOW_NCPAINT = 0x4,
	WCA_CAPTION_BUTTON_BOUNDS = 0x5,
	WCA_NONCLIENT_RTL_LAYOUT = 0x6,
	WCA_FORCE_ICONIC_REPRESENTATION = 0x7,
	WCA_EXTENDED_FRAME_BOUNDS = 0x8,
	WCA_HAS_ICONIC_BITMAP = 0x9,
	WCA_THEME_ATTRIBUTES = 0xA,
	WCA_NCRENDERING_EXILED = 0xB,
	WCA_NCADORNMENTINFO = 0xC,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 0xD,
	WCA_VIDEO_OVERLAY_ACTIVE = 0xE,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 0xF,
	WCA_DISALLOW_PEEK = 0x10,
	WCA_CLOAK = 0x11,
	WCA_CLOAKED = 0x12,
	WCA_ACCENT_POLICY = 0x13,
	WCA_FREEZE_REPRESENTATION = 0x14,
	WCA_EVER_UNCLOAKED = 0x15,
	WCA_VISUAL_OWNER = 0x16,
	WCA_HOLOGRAPHIC = 0x17,
	WCA_EXCLUDED_FROM_DDA = 0x18,
	WCA_PASSIVEUPDATEMODE = 0x19,
	WCA_LAST = 0x1A,
};

typedef struct WINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attrib;
	void* pvData;
	DWORD cbData;
};

typedef BOOL(WINAPI* SetWindowCompositionAttribute)(
	IN HWND hwnd,
	IN WINDOWCOMPOSITIONATTRIBDATA* pwcad);

//Windows::UI::Composition::IInteropCompositorPartner
DECLARE_INTERFACE_IID_(IInteropCompositorPartner, IUnknown, "e7894c70-af56-4f52-b382-4b3cd263dc6f")
{
	STDMETHOD(MarkDirty)(THIS_) PURE;

	STDMETHOD(ClearCallback)(THIS_) PURE;

	STDMETHOD(CreateManipulationTransform)(THIS_
		IN IDCompositionTransform * transform,
		IN REFIID iid,
		OUT VOID * *result) PURE;

	STDMETHOD(RealClose)(THIS_) PURE;
};

//Windows.UI.Composition.IInteropCompositorPartnerCallback
DECLARE_INTERFACE_IID_(IInteropCompositorPartnerCallback, IUnknown, "9bb59fc9-3326-4c32-bf06-d6b415ac2bc5")
{
	STDMETHOD(NotifyDirty)(THIS_) PURE;

	STDMETHOD(NotifyDeferralState)(THIS_
		bool deferRequested) PURE;
};

//Windows::UI::Composition::IInteropCompositorFactoryPartner
DECLARE_INTERFACE_IID_(IInteropCompositorFactoryPartner, IInspectable, "22118adf-23f1-4801-bcfa-66cbf48cc51b")
{
	STDMETHOD(CreateInteropCompositor)(THIS_
		IN IUnknown * renderingDevice,
		IN IInteropCompositorPartnerCallback * callback,
		IN REFIID iid,
		OUT VOID * *instance
		) PURE;

	STDMETHOD(CheckEnabled)(THIS_
		OUT bool* enableInteropCompositor,
		OUT bool* enableExposeVisual
		) PURE;
};