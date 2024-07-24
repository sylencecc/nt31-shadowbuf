/******************************Module*Header*******************************\
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
\**************************************************************************/

#include "driver.h"

// The driver function table with all function index/address pairs
static DRVFN gadrvfn[] =
{
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode         },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette         },
    {   INDEX_DrvMovePointer,           (PFN) DrvMovePointer        },
    {   INDEX_DrvSetPointerShape,       (PFN) DrvSetPointerShape    },
    {   INDEX_DrvDitherColor,           (PFN) DrvDitherColor        },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits           },
    {   INDEX_DrvStretchBlt,            (PFN) DrvStretchBlt         },
    {   INDEX_DrvPlgBlt,                (PFN) DrvPlgBlt             },
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut            },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath         },
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt             },
    {   INDEX_DrvPaint,                 (PFN) DrvPaint              },
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath           },
    {   INDEX_DrvStrokeAndFillPath,     (PFN) DrvStrokeAndFillPath  },
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes           }
};

// Hook all available surface functions
#define HOOKS HOOK_BITBLT | HOOK_COPYBITS | HOOK_STRETCHBLT | HOOK_PLGBLT | HOOK_TEXTOUT | HOOK_PAINT | HOOK_STROKEPATH | HOOK_FILLPATH | HOOK_STROKEANDFILLPATH

/******************************Public*Routine******************************\
* DrvEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
\**************************************************************************/

BOOL DrvEnableDriver(
ULONG iEngineVersion,
ULONG cj,
PDRVENABLEDATA pded)
{
    // Engine Version is passed down so future drivers can support previous
    // engine versions.  A next generation driver can support both the old
    // and new engine conventions if told what version of engine it is
    // working with.  For the first version the driver does nothing with it.

    iEngineVersion;

    // Fill in as much as we can.

    if (cj >= sizeof(DRVENABLEDATA))
        pded->pdrvfn = gadrvfn;

    if (cj >= (sizeof(ULONG) * 2))
        pded->c = sizeof(gadrvfn) / sizeof(DRVFN);

    // DDI version this driver was targeted for is passed back to engine.
    // Future graphic's engine may break calls down to old driver format.

    if (cj >= sizeof(ULONG))
        pded->iDriverVersion = DDI_DRIVER_VERSION;

    return(TRUE);
}

/******************************Public*Routine******************************\
* DrvDisableDriver
*
* Tells the driver it is being disabled. Release any resources allocated in
* DrvEnableDriver.
*
\**************************************************************************/

VOID DrvDisableDriver(VOID)
{
    return;
}

/******************************Public*Routine******************************\
* DrvEnablePDEV
*
* DDI function, Enables the Physical Device.
*
* Return Value: device handle to pdev.
*
\**************************************************************************/

DHPDEV DrvEnablePDEV(
DEVMODEW   *pDevmode,       // Pointer to DEVMODE
PWSTR       pwszLogAddress, // Logical address
ULONG       cPatterns,      // number of patterns
HSURF      *ahsurfPatterns, // return standard patterns
ULONG       cjGdiInfo,      // Length of memory pointed to by pGdiInfo
ULONG      *pGdiInfo,       // Pointer to GdiInfo structure
ULONG       cjDevInfo,      // Length of following PDEVINFO structure
DEVINFO    *pDevInfo,       // physical device information structure
PWSTR       pwszDataFile,   // DataFile - not used
PWSTR       pwszDeviceName, // DeviceName - not used
HANDLE      hDriver)        // Handle to base driver
{
    GDIINFO GdiInfo;
    DEVINFO DevInfo;
    PPDEV   ppdev = (PPDEV) NULL;

    UNREFERENCED_PARAMETER(pwszLogAddress);
    UNREFERENCED_PARAMETER(pwszDataFile);
    UNREFERENCED_PARAMETER(pwszDeviceName);

    // Allocate a physical device structure.

    ppdev = (PPDEV) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(PDEV));

    if (ppdev == (PPDEV) NULL)
    {
        RIP("DISP DrvEnablePDEV failed LocalAlloc\n");
        return((DHPDEV) 0);
    }

    // Save the screen handle in the PDEV.

    ppdev->hDriver = hDriver;

    // Get the current screen mode information.  Set up device caps and devinfo.

    if (!bInitPDEV(ppdev, pDevmode, &GdiInfo, &DevInfo))
    {
        DISPDBG((1,"DISP DrvEnablePDEV failed\n"));
        goto error_free;
    }

    // Initialize the cursor information.

    if (!bInitPointer(ppdev, &DevInfo))
    {
        // Not a fatal error...
        DISPDBG((0, "DISP DrvEnableSurface failed bInitPointer\n"));
    }

    // Initialize palette information.

    if (!bInitPaletteInfo(ppdev, &DevInfo))
    {
        RIP("DISP DrvEnableSurface failed bInitPalette\n");
        goto error_free;
    }

    // Initialize device standard patterns.

    if (!bInitPatterns(ppdev, min(cPatterns, HS_DDI_MAX)))
    {
        RIP("DISP DrvEnablePDEV failed bInitPatterns\n");
        vDisablePatterns(ppdev);
        vDisablePalette(ppdev);
        goto error_free;
    }

    // Copy the devinfo into the engine buffer.

    memcpy(pDevInfo, &DevInfo, min(sizeof(DEVINFO), cjDevInfo));

    // Set the ahsurfPatterns array to handles each of the standard
    // patterns that were just created.

    memcpy((PVOID)ahsurfPatterns, ppdev->ahbmPat, ppdev->cPatterns*sizeof(HBITMAP));

    // Set the pdevCaps with GdiInfo we have prepared to the list of caps for this
    // pdev.

    memcpy(pGdiInfo, &GdiInfo, min(cjGdiInfo, sizeof(GDIINFO)));

    return((DHPDEV) ppdev);

    // Error case for failure.
error_free:
    LocalFree(ppdev);
    RIP("DISP DrvEnablePDEV failed\n");
    return((DHPDEV) 0);
}

/******************************Public*Routine******************************\
* DrvCompletePDEV
*
* Store the HPDEV, the engines handle for this PDEV, in the DHPDEV.
*
\**************************************************************************/

VOID DrvCompletePDEV(
DHPDEV dhpdev,
HDEV  hdev)
{
    ((PPDEV) dhpdev)->hdevEng = hdev;
}

/******************************Public*Routine******************************\
* DrvDisablePDEV
*
* Release the resources allocated in DrvEnablePDEV.  If a surface has been
* enabled DrvDisableSurface will have already been called.
*
\**************************************************************************/

VOID DrvDisablePDEV(
DHPDEV dhpdev)
{
    vDisablePalette((PPDEV) dhpdev);
    vDisablePatterns((PPDEV) dhpdev);
    LocalFree(dhpdev);
}

/******************************Public*Routine******************************\
* DrvEnableSurface
*
* Enable the surface for the device.  Hook the calls this driver supports.
*
* Return: Handle to the surface if successful, 0 for failure.
*
\**************************************************************************/

HSURF DrvEnableSurface(
DHPDEV dhpdev)
{
    PPDEV ppdev;
    HSURF hsurf;
    SIZEL sizl;
    ULONG ulBitmapType;

    // Create engine bitmap around frame buffer.

    ppdev = (PPDEV) dhpdev;

    if (!bInitSURF(ppdev, TRUE))
    {
        RIP("DISP DrvEnableSurface failed bInitSURF\n");
        return(FALSE);
    }

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    if (ppdev->ulBitCount == 8)
    {
        if (!bInit256ColorPalette(ppdev)) {
            RIP("DISP DrvEnableSurface failed to init the 8bpp palette\n");
            return(FALSE);
        }
        ulBitmapType = BMF_8BPP;
    }
    else if (ppdev->ulBitCount == 16)
    {
        ulBitmapType = BMF_16BPP;
    }
    else if (ppdev->ulBitCount == 24)
    {
        ulBitmapType = BMF_24BPP;
    }
    else
    {
        ulBitmapType = BMF_32BPP;
    }

	  // Create an engine-managed bitmap (DIB) that serves as shadow buffer
    hsurf = (HSURF) EngCreateBitmap(sizl,
                                    ppdev->lDeltaScreen,
                                    ulBitmapType,
                                    (ppdev->lDeltaScreen > 0) ? BMF_TOPDOWN : 0,
                                    NULL);
    if (hsurf == (HSURF) 0)
    {
        RIP("DISP DrvEnableSurface failed EngCreateBitmap\n");
        return(FALSE);
    }

    // Create an engine-managed bitmap (DIB) for the actual device framebuffer
    ppdev->hsurfBuffer = (HSURF) EngCreateBitmap(sizl,
                                            ppdev->lDeltaScreen,
                                            ulBitmapType,
                                            (ppdev->lDeltaScreen > 0) ? BMF_TOPDOWN : 0,
                                            (PVOID) (ppdev->pjScreen));
    if (ppdev->hsurfBuffer == (HSURF) 0)
    {
        RIP("DISP DrvEnableSurface failed EngCreateBitmap for buffer\n");
        return(FALSE);
    }

    if (!EngAssociateSurface(hsurf, ppdev->hdevEng, HOOKS))
    {
        RIP("DISP DrvEnableSurface failed EngAssociateSurface\n");
        EngDeleteSurface(hsurf);
        return(FALSE);
    }

    ppdev->hsurfEng = hsurf;

    return(hsurf);
}

/******************************Public*Routine******************************\
* DrvDisableSurface
*
* Free resources allocated by DrvEnableSurface.  Release the surface.
*
\**************************************************************************/

VOID DrvDisableSurface(
DHPDEV dhpdev)
{
    EngDeleteSurface(((PPDEV) dhpdev)->hsurfEng);
    EngDeleteSurface(((PPDEV) dhpdev)->hsurfBuffer);
    vDisableSURF((PPDEV) dhpdev);
    ((PPDEV) dhpdev)->hsurfEng = (HSURF) 0;
}

/******************************Public*Routine******************************\
* DrvAssertMode
*
* This asks the device to reset itself to the mode of the pdev passed in.
*
\**************************************************************************/

VOID DrvAssertMode(
DHPDEV dhpdev,
BOOL bEnable)
{
    PPDEV   ppdev = (PPDEV) dhpdev;
    ULONG   ulReturn;

    if (bEnable)
    {
        // The screen must be reenabled, reinitialize the device to clean state.
        bInitSURF(ppdev, FALSE);
    }
    else
    {
        // We must give up the display.
        // Call the kernel driver to reset the device to a known state.
        if (!DeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_RESET_DEVICE,
                             NULL,
                             0,
                             NULL,
                             0,
                             &ulReturn,
                             NULL))
        {
            RIP("DISP DrvAssertMode failed IOCTL");
        }
    }

    return;
}

/******************************Public*Routine******************************\
* DrvGetModes
*
* Returns the list of available modes for the device.
*
\**************************************************************************/

ULONG DrvGetModes(
HANDLE hDriver,
ULONG cjSize,
DEVMODEW *pdm)

{

    DWORD cModes;
    DWORD cbOutputSize;
    PVIDEO_MODE_INFORMATION pVideoModeInformation, pVideoTemp;
    DWORD cOutputModes = cjSize / (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    DWORD cbModeSize;

    DISPDBG((3, "Framebuf.dll:DrvGetModes\n"));

    cModes = getAvailableModes(hDriver,
                               (PVIDEO_MODE_INFORMATION *) &pVideoModeInformation,
                               &cbModeSize);

    if (cModes == 0)
    {
        DISPDBG((0, "FRAMEBUF DISP DrvGetModes failed to get mode information"));
        return 0;
    }

    if (pdm == NULL)
    {
        cbOutputSize = cModes * (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    }
    else
    {
        //
        // Now copy the information for the supported modes back into the output
        // buffer
        //

        cbOutputSize = 0;

        pVideoTemp = pVideoModeInformation;

        do
        {
            if (pVideoTemp->Length != 0)
            {
                if (cOutputModes == 0)
                {
                    break;
                }

                //
                // Zero the entire structure to start off with.
                //

                memset(pdm, 0, sizeof(DEVMODEW));

                //
                // Set the name of the device to the name of the DLL.
                //

                memcpy(&(pdm->dmDeviceName), L"framebuf", sizeof(L"framebuf"));

                pdm->dmSpecVersion = DM_SPECVERSION;
                pdm->dmDriverVersion = DM_SPECVERSION;

                //
                // We currently do not support Extra information in the driver
                //

                pdm->dmDriverExtra = DRIVER_EXTRA_SIZE;

                pdm->dmSize = sizeof(DEVMODEW);
                pdm->dmBitsPerPel = pVideoTemp->NumberOfPlanes *
                                    pVideoTemp->BitsPerPlane;
                pdm->dmPelsWidth = pVideoTemp->VisScreenWidth;
                pdm->dmPelsHeight = pVideoTemp->VisScreenHeight;
                pdm->dmDisplayFrequency = pVideoTemp->Frequency;

                if (pVideoTemp->AttributeFlags & VIDEO_MODE_INTERLACED)
                {
                    pdm->dmDisplayFlags |= DM_INTERLACED;
                }

                //
                // Go to the next DEVMODE entry in the buffer.
                //

                cOutputModes--;

                pdm = (LPDEVMODEW) ( ((ULONG)pdm) + sizeof(DEVMODEW) +
                                                   DRIVER_EXTRA_SIZE);

                cbOutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);

            }

            pVideoTemp = (PVIDEO_MODE_INFORMATION)
                (((PUCHAR)pVideoTemp) + cbModeSize);

        } while (--cModes);
    }

    LocalFree(pVideoModeInformation);

    return cbOutputSize;

}

/*----------------------------------------------------------------------------
 Buffering to speed up DrvCopyBits() and *Blt()

 We hook all available surface drawing functions and cause each function to
 call its respective Eng* function first to draw onto the actual framebuffer,
 followed by drawing onto the in-memory cache bitmap. Since fetching memory
 from the actual framebuffer is slow, especially on real hardware, we
 prevent that bottleneck by letting DrvCopyBits() and *Blt() functions
 fetch from the in-memory cache bitmap instead.
----------------------------------------------------------------------------*/

BOOL DrvCopyBits(
SURFOBJ  *psoTrg,
SURFOBJ  *psoSrc,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclTrg,
POINTL   *pptlSrc)
{
    HSURF psoBuffer;
    psoBuffer = EngLockSurface(((PPDEV) psoTrg->dhpdev)->hsurfBuffer);
    EngCopyBits(psoBuffer, psoSrc, pco, pxlo, prclTrg, pptlSrc);
    EngUnlockSurface(psoBuffer);
    return EngCopyBits(psoTrg, psoSrc, pco, pxlo, prclTrg, pptlSrc);
}

BOOL DrvStretchBlt(
SURFOBJ *psoDest,
SURFOBJ *psoSrc,
SURFOBJ *psoMask,
CLIPOBJ *pco,
XLATEOBJ *pxlo,
COLORADJUSTMENT *pca,
POINTL *pptlHTOrg,
RECTL *prclDest,
RECTL *prclSrc,
POINTL *pptlMask,
ULONG iMode
)
{
    HSURF psoBuffer;
    psoBuffer = EngLockSurface(((PPDEV) psoDest->dhpdev)->hsurfBuffer);
    EngStretchBlt(psoBuffer, psoSrc, psoMask, pco, pxlo, pca, pptlHTOrg, prclDest, prclSrc, pptlMask, iMode);
    EngUnlockSurface(psoBuffer);
    return EngStretchBlt(psoDest, psoSrc, psoMask, pco, pxlo, pca, pptlHTOrg, prclDest, prclSrc, pptlMask,iMode);
}

BOOL DrvPlgBlt(
SURFOBJ *psoDest,
SURFOBJ *psoSrc,
SURFOBJ *psoMask,
CLIPOBJ *pco,
XLATEOBJ *pxlo,
COLORADJUSTMENT *pca,
POINTL *pptlHTOrg,
POINTFIX *ppfixDest,
RECTL *prclSrc,
POINTL *pptlMask,
ULONG iMode)
{
    HSURF psoBuffer;
	  psoBuffer = EngLockSurface(((PPDEV) psoDest->dhpdev)->hsurfBuffer);
	  EngPlgBlt(psoBuffer, psoSrc, psoMask, pco, pxlo, pca, pptlHTOrg, ppfixDest, prclSrc, pptlMask, iMode);
	  EngUnlockSurface(psoBuffer);
	  return EngPlgBlt(psoDest, psoSrc, psoMask, pco, pxlo, pca, pptlHTOrg, ppfixDest, prclSrc, pptlMask, iMode);
}

BOOL DrvTextOut(
SURFOBJ  *pso,
STROBJ   *pstro,
FONTOBJ  *pfo,
CLIPOBJ  *pco,
RECTL    *prclExtra,
RECTL    *prclOpaque,
BRUSHOBJ *pboFore,
BRUSHOBJ *pboOpaque,
POINTL   *pptlOrg,
MIX       mix)
{
    HSURF psoBuffer;
    // Calling EngTextOut() is destructive, because it modifies pco.
    // Since we need to call it twice, we OR in OC_BANK_CLIP (undocumented),
    // which causes bBanked within EngTextOut() to be set to True.
    // Consequently, EngTextOut() will revert all changes to pco.
    // See NT 3.5 sources (TEXTBLT.CXX) for details.
    pco->fjOptions |= OC_BANK_CLIP;
    psoBuffer = EngLockSurface(((PPDEV) pso->dhpdev)->hsurfBuffer);
    EngTextOut(psoBuffer, pstro, pfo, pco, prclExtra, prclOpaque, pboFore, pboOpaque, pptlOrg, mix);
    EngUnlockSurface(psoBuffer);
    return EngTextOut(pso, pstro, pfo, pco, prclExtra, prclOpaque, pboFore, pboOpaque, pptlOrg, mix);
}

BOOL DrvStrokePath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pbo,
POINTL    *pptlBrushOrg,
LINEATTRS *plineattrs,
MIX        mix)
{
    HSURF psoBuffer;
    psoBuffer = EngLockSurface(((PPDEV) pso->dhpdev)->hsurfBuffer);
    EngStrokePath(psoBuffer, ppo, pco, pxo, pbo, pptlBrushOrg, plineattrs, mix);
    EngUnlockSurface(psoBuffer);
    return EngStrokePath(pso, ppo, pco, pxo, pbo, pptlBrushOrg, plineattrs, mix);
}

BOOL DrvBitBlt(
SURFOBJ *psoTrg,
SURFOBJ *psoSrc,
SURFOBJ *psoMask,
CLIPOBJ *pco,
XLATEOBJ *pxlo,
RECTL *prclTrg,
POINTL *pptlSrc,
POINTL *pptlMask,
BRUSHOBJ *pbo,
POINTL *pptlBrush,
ROP4 rop4)
{
    HSURF psoBuffer;
    psoBuffer = EngLockSurface(((PPDEV) psoTrg->dhpdev)->hsurfBuffer);
    EngBitBlt(psoBuffer, psoSrc, psoMask, pco, pxlo, prclTrg, pptlSrc, pptlMask, pbo, pptlBrush, rop4);
    EngUnlockSurface(psoBuffer);
    return EngBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo, prclTrg, pptlSrc, pptlMask, pbo, pptlBrush, rop4);
}

BOOL DrvPaint(
SURFOBJ *pso,
CLIPOBJ *pco,
BRUSHOBJ *pbo,
POINTL *pptlBrushOrg,
MIX mix)
{
    HSURF psoBuffer;
    psoBuffer = EngLockSurface(((PPDEV) pso->dhpdev)->hsurfBuffer);
    EngPaint(psoBuffer, pco, pbo, pptlBrushOrg, mix);
    EngUnlockSurface(psoBuffer);
    return EngPaint(pso, pco, pbo, pptlBrushOrg, mix);
}

BOOL DrvFillPath(
SURFOBJ *pso,
PATHOBJ *ppo,
CLIPOBJ *pco,
BRUSHOBJ *pbo,
POINTL *pptlBrushOrg,
MIX mix,
FLONG flOptions)
{
    HSURF psoBuffer;
    psoBuffer = EngLockSurface(((PPDEV) pso->dhpdev)->hsurfBuffer);
    EngFillPath(psoBuffer, ppo, pco, pbo, pptlBrushOrg, mix, flOptions);
    EngUnlockSurface(psoBuffer);
    return EngFillPath(pso, ppo, pco, pbo, pptlBrushOrg, mix, flOptions);
}

BOOL DrvStrokeAndFillPath(
SURFOBJ *pso,
PATHOBJ *ppo,
CLIPOBJ *pco,
XFORMOBJ *pxo,
BRUSHOBJ *pboStroke,
LINEATTRS *plineattrs,
BRUSHOBJ *pboFill,
POINTL *pptlBrushOrg,
MIX mixFill,
FLONG flOptions)
{
    HSURF psoBuffer;
    psoBuffer = EngLockSurface(((PPDEV) pso->dhpdev)->hsurfBuffer);
    EngStrokeAndFillPath(psoBuffer, ppo, pco, pxo, pboStroke, plineattrs, pboFill, pptlBrushOrg, mixFill, flOptions);
    EngUnlockSurface(psoBuffer);
    return EngStrokeAndFillPath(pso, ppo, pco, pxo, pboStroke, plineattrs, pboFill, pptlBrushOrg, mixFill, flOptions);
}
