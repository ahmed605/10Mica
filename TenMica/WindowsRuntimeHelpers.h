#pragma once

#include <tuple>
#include <dcomp.local.h>

using namespace Microsoft::WRL::Wrappers;

typedef HRESULT(WINAPI* CreateFloodEffectFunc)(
	IN void* thisPtr, OUT IDCompositionFloodEffect** floodEffect);

typedef HRESULT(WINAPI* CreateVisualSurfaceFunc)(
	IN void* thisPtr, OUT ICompositionVisualSurfaceLegacy^* visualSurface);

const GUID IID_IDCompositionDesktopDevicePartner3_1703 = { 0x0ab6bdb3, 0x4d49, 0x46a8, { 0xb9, 0x0b, 0x1a, 0x86, 0xb0, 0xcd, 0x4e, 0x41 } };
const GUID IID_IDCompositionDesktopDevicePartner4_1709 = { 0x0D2037FF5, 0x0F28, 0x4D60, { 0xAC, 0x8D, 0xCE, 0xE2, 0x4C, 0xB0, 0x72, 0xE3 } };
const GUID IID_IDCompositionDesktopDevicePartner5_1803 = { 0x0CB139649, 0x6D80, 0x48E7, { 0xB5, 0x4D, 9, 0x73, 0x7D, 0x84, 0xDB, 0x47 } };
const GUID IID_ICompositorPartner2_1703 = { 0xdcead379, 0xd98d, 0x486a, { 0x85, 0x06, 0x38, 0xc4, 0xda, 0x7f, 0xc1, 0x9e } };
const GUID IID_ICompositorPartner2_1803 = { 0x0E5AC2423, 0x0F290, 0x47B3, { 0x80, 0xD3, 0x55, 0x24, 0xFE, 0x12, 0x61, 0x76 } };

struct VtblStruct
{
public:
	void** vtbl;
};

inline std::tuple<void*, void*, CreateFloodEffectFunc, CreateVisualSurfaceFunc> GetFloodEffectAndVisualSurfaceFunctions(IDCompositionDevice3* device)
{
	VtblStruct* dStruct;
	VtblStruct* cStruct;
	CreateFloodEffectFunc floodEffectFunc = nullptr;
	CreateVisualSurfaceFunc visualSurfaceFunc = nullptr;

	if (device->QueryInterface(IID_IDCompositionDesktopDevicePartner3_1703, (void**)&dStruct) == S_OK // 1703
	 && device->QueryInterface(IID_ICompositorPartner2_1703, (void**)&cStruct) == S_OK)
	{
		floodEffectFunc = (CreateFloodEffectFunc)dStruct->vtbl[66];
		visualSurfaceFunc = (CreateVisualSurfaceFunc)cStruct->vtbl[6];
	}
	else if (device->QueryInterface(IID_IDCompositionDesktopDevicePartner5_1803, (void**)&dStruct) == S_OK) // 1803 - 1809
	{
		floodEffectFunc = (CreateFloodEffectFunc)dStruct->vtbl[65];

		if (device->QueryInterface(IID_ICompositorPartner2_1803, (void**)&cStruct) == S_OK)
		{
			wchar_t* end;
			auto familyVersion = Windows::System::Profile::AnalyticsInfo::VersionInfo->DeviceFamilyVersion;
			auto v = wcstoull(familyVersion->Data(), &end, 10);

			auto build = (v & 0x00000000FFFF0000L) >> 16;
			visualSurfaceFunc = (CreateVisualSurfaceFunc)cStruct->vtbl[build >= 17763 ? 22 : 19];
		}
	}
	else if (device->QueryInterface(IID_IDCompositionDesktopDevicePartner4_1709, (void**)&dStruct) == S_OK // 1709
		  && device->QueryInterface(IID_ICompositorPartner2_1703, (void**)&cStruct) == S_OK)
	{
		floodEffectFunc = (CreateFloodEffectFunc)dStruct->vtbl[65];
		visualSurfaceFunc = (CreateVisualSurfaceFunc)cStruct->vtbl[9];
	}

	return std::make_tuple(dStruct, cStruct, floodEffectFunc, visualSurfaceFunc);
}

inline ICompositionVisualSurfaceLegacy^ GetVisualSurface(void* thisPtr, CreateVisualSurfaceFunc func)
{
	ICompositionVisualSurfaceLegacy^ surface = nullptr;
	func(thisPtr, &surface);

	return surface;
}

template <typename T>
inline bool VectorViewHasValue(Windows::Foundation::Collections::IVectorView<T>^ view, bool func(T))
{
	for (T item : view)
		if (func(item)) return true;

	return false;
}