
//---------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//---------------------------------------------------------------------------

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <sdkddkver.h>
#include <Windows.h>
#using "Microsoft.UI.winmd"

using namespace Microsoft::UI::Composition;

#ifndef BUILD_WINDOWS
namespace ABI {
#endif

namespace Microsoft {
namespace UI {
namespace Composition {

typedef interface ICompositionDrawingSurfaceInterop ICompositionDrawingSurfaceInterop;
typedef interface ICompositionDrawingSurfaceInterop2 ICompositionDrawingSurfaceInterop2;
typedef interface ICompositorInterop ICompositorInterop;


#undef INTERFACE
#define INTERFACE ICompositionDrawingSurfaceInterop
DECLARE_INTERFACE_IID_(ICompositionDrawingSurfaceInterop, IUnknown, "2D6355C2-AD57-4EAE-92E4-4C3EFF65D578")
{
    IFACEMETHOD(BeginDraw)(
        _In_opt_ const RECT * updateRect,
        _In_ REFIID iid,
        _COM_Outptr_ void ** updateObject,
        _Out_ POINT * updateOffset
        ) PURE;

    IFACEMETHOD(EndDraw)(
        ) PURE;

    IFACEMETHOD(Resize)(
        _In_ SIZE sizePixels
        ) PURE;

    IFACEMETHOD(Scroll)(
        _In_opt_ const RECT * scrollRect,
        _In_opt_ const RECT * clipRect,
        _In_ int offsetX,
        _In_ int offsetY
        ) PURE;

    IFACEMETHOD(ResumeDraw)(
        ) PURE;

    IFACEMETHOD(SuspendDraw)(
        ) PURE;
};

#undef INTERFACE
#define INTERFACE ICompositionDrawingSurfaceInterop2
DECLARE_INTERFACE_IID_(ICompositionDrawingSurfaceInterop2, ICompositionDrawingSurfaceInterop, "D4B71A65-3052-4ABE-9183-E98DE02A41A9")
{
    IFACEMETHOD(CopySurface)(
        _In_ IUnknown* destinationResource,
        _In_ int destinationOffsetX,
        _In_ int destinationOffsetY,
        _In_opt_ const RECT * sourceRectangle
        ) PURE;
};

#undef INTERFACE
#define INTERFACE ICompositionGraphicsDeviceInterop
DECLARE_INTERFACE_IID_(ICompositionGraphicsDeviceInterop, IUnknown, "4AFA8030-BC70-4B0C-B1C7-6E69F933DC83")
{
    IFACEMETHOD(GetRenderingDevice)(
        _COM_Outptr_ IUnknown ** value
        ) PURE;

    IFACEMETHOD(SetRenderingDevice)(
        _In_ IUnknown * value
        ) PURE;
};


#undef INTERFACE
#define INTERFACE ICompositorInterop
DECLARE_INTERFACE_IID_(ICompositorInterop, IUnknown, "FAB19398-6D19-4D8A-B752-8F096C396069")
{
    IFACEMETHOD(CreateGraphicsDevice)(
        _In_ IUnknown * renderingDevice,
        _COM_Outptr_ ICompositionGraphicsDevice^* result
        ) PURE;
};

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

namespace Interactions
{

#undef INTERFACE
#define INTERFACE IVisualInteractionSourceInterop
DECLARE_INTERFACE_IID_(IVisualInteractionSourceInterop, IUnknown, "AA170AEE-01D7-4954-89D2-8554415D6946")
{
    IFACEMETHOD(TryRedirectForManipulation)(
        _In_ const POINTER_INFO& pointerInfo
        ) PURE;
};

} // Interactions

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion


} // namespace Composition
} // namespace UI
} // namespace Microsoft
#ifndef BUILD_WINDOWS
} // namespace ABI 
#endif
