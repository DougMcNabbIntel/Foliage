//--------------------------------------------------------------------------------------
// Copyright 2013 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//--------------------------------------------------------------------------------------
#ifndef __FOLIAGE_H__
#define __FOLIAGE_H__

#include "CPUT_DX11.h"
#include <stdio.h>
#include <D3D11.h>
#include <xnamath.h>
#include <time.h>
#include "CPUTSprite.h"
#include "CPUTFoliagePatch.h"
#include "CPUTPostProcess.h"


// define some controls
const CPUTControlID ID_MAIN_PANEL = 10;
const CPUTControlID ID_SECONDARY_PANEL = 20;
const CPUTControlID ID_FULLSCREEN_BUTTON = 100;
const CPUTControlID ID_VSYNC_CHECKBOX    = 101;
const CPUTControlID ID_STATIC_TEXT_TEST  = 102;
const CPUTControlID ID_IGNORE_CONTROL_ID = -1;

class CPUTPostProcess;

//-----------------------------------------------------------------------------
class MySample : public CPUT_DX11
{
private:
    CPUTAssetSet          *mpTerrainSet;
    CPUTAssetSet          *mpSkySet;
    CPUTCameraController  *mpCameraController;
    CPUTCameraController  *mpShadowCameraController;

    CPUTAssetSet          *mpShadowCameraSet;
    CPUTRenderTargetDepth *mpShadowRenderTarget;
    CPUTRenderTargetColor *mpTerrainLightMapRenderTarget;

    CPUTRenderTargetColor *mpMSAABackBuffer;
    CPUTRenderTargetDepth *mpMSAADepthBuffer;

    CPUTPostProcess       *mpPostProcess;

    CPUTText              *mpFPSCounter;
    CPUTFoliagePatchDX11  *mpFoliagePatch;

    CPUTRenderTargetColor *mpBillboardRenderTargetColor;
    CPUTRenderTargetColor *mpBillboardRenderTargetNormal;
    CPUTRenderTargetDepth *mpBillboardRenderTargetDepth;

public:
    MySample() : 
        mpTerrainSet(NULL),
        mpSkySet(NULL),
        mpCameraController(NULL),
        mpShadowCameraController(NULL),
        mpShadowCameraSet(NULL),
        mpMSAABackBuffer(NULL),
        mpMSAADepthBuffer(NULL),
        mpPostProcess(NULL),
        mpFoliagePatch(NULL),
        mpBillboardRenderTargetColor(NULL),
        mpBillboardRenderTargetNormal(NULL),
        mpBillboardRenderTargetDepth(NULL),
		mpTerrainLightMapRenderTarget(NULL)
    {}
    virtual ~MySample()
    {
        // Note: these two are defined in the base.  We release them because we addref them.
        SAFE_RELEASE(mpCamera);
        SAFE_RELEASE(mpShadowCamera);

        SAFE_RELEASE(mpTerrainSet);
        SAFE_RELEASE(mpSkySet);
        SAFE_DELETE( mpCameraController );
        SAFE_DELETE( mpShadowCameraController );
        SAFE_RELEASE(mpShadowCameraSet);
        SAFE_DELETE( mpShadowRenderTarget );
        SAFE_DELETE( mpTerrainLightMapRenderTarget );
        SAFE_DELETE( mpMSAABackBuffer);
        SAFE_DELETE( mpMSAADepthBuffer);
        SAFE_DELETE( mpPostProcess );
        SAFE_RELEASE(mpFoliagePatch );
        SAFE_DELETE( mpBillboardRenderTargetColor );
        SAFE_DELETE( mpBillboardRenderTargetNormal );
        SAFE_DELETE( mpBillboardRenderTargetDepth );
    }
    CPUTEventHandledCode HandleKeyboardEvent(CPUTKey key);
    CPUTEventHandledCode HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state);
    void                 HandleCallbackEvent( CPUTEventID Event, CPUTControlID ControlID, CPUTControl *pControl );

    void Create();
    void Render(double deltaSeconds);
    void Update(double deltaSeconds);
    void ResizeWindow(UINT width, UINT height);
    void CreateCameras();
};
#endif // __FOLIAGE_H__
