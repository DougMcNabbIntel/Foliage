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

#define GENERATE_BILLBOARD_TEXTURES 0
#define BAKE_TERRAIN_TEXTURE        0
bool    gUpdateShadows = true;
bool    gEnableHDR     = true;

float   gBillboardDistance;

extern bool gRowMajorColMajorNot;
extern bool gInvertX;
extern bool gInvertY;

#ifdef DEBUG
    #define _CRT_DEBUG()
    #define _CRTDBG_MAP_ALLOC
    #define _CRTDBG_MAP_ALLOC_NEW
    #include <stdlib.h>
    #include <crtdbg.h>
    #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
    #define new DBG_NEW
#endif

#include "Foliage.h"
#include "CPUTRenderTarget.h"
#include "CPUTFoliagePatch.h"
#include "CPUTPostProcess.h"

const UINT SHADOW_WIDTH_HEIGHT            = 8192;;
const UINT TERRAIN_LIGHT_MAP_WIDTH_HEIGHT = 4096;

extern float3 gLightDir;
extern char *gpDefaultShaderSource;
bool gShowBoundingBoxes = false;
cString gFilename;
int gCheckerBoard = false;

// Application entry point.  Execution begins here.
//-----------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Prevent unused parameter compiler warnings
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

#ifdef DEBUG
    // tell VS to report leaks at any exit of the program
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
    CPUTResult result=CPUT_SUCCESS;
    int returnCode=0;

    // create an instance of my sample
    MySample *pSample = new MySample();
    
    // We make the assumption we are running from the executable's dir in
    // the executable directory or it won't be able to use the relative paths to find the default
    // resources    
    cString ResourceDirectory;
    CPUTOSServices::GetOSServices()->GetExecutableDirectory(&ResourceDirectory);
    ResourceDirectory.append(_L("..\\CPUT\\resources\\"));
    
    // Initialize the system and give it the base CPUT resource directory (location of GUI images/etc)
    // For now, we assume it's a relative directory from the executable directory.  Might make that resource
    // directory location an env variable/hardcoded later
    pSample->CPUTInitialize(ResourceDirectory); 

    // window and device parameters
    CPUTWindowCreationParams params;
    params.deviceParams.refreshRate         = 0;
    params.deviceParams.swapChainBufferCount= 1;
    params.deviceParams.swapChainFormat     = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    params.deviceParams.swapChainUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;

    // parse out the parameter settings or reset them to defaults if not specified
    cString CommandLine(lpCmdLine);
    pSample->CPUTParseCommandLine(CommandLine, &params, &gFilename);       

    // create the window and device context
    result = pSample->CPUTCreateWindowAndContext(_L("Foliage Sample"), params);
    ASSERT( CPUTSUCCESS(result), _L("CPUT Error creating window and context.") );

    // start the main message loop
    returnCode = pSample->CPUTMessageLoop();

    pSample->DeviceShutdown();

    // cleanup resources
    SAFE_DELETE(pSample);

    return returnCode;
}

//-----------------------------------------------------------------------------
void MySample::CreateCameras()
{
    // If no cameras were created from the model sets then create a default simple camera
    if( mpTerrainSet && mpTerrainSet->GetCameraCount() )
    {
        mpCamera = mpTerrainSet->GetFirstCamera();
        mpCamera->AddRef(); // TODO: why?  Shouldn't need this.
    } else
    {
        mpCamera = new CPUTCamera();
        CPUTAssetLibraryDX11::GetAssetLibrary()->AddCamera( _L("Camera"), mpCamera );

        float4x4 cameraMatrix(
              0.82577485f,   0.00000000f,   0.56400007f, 0.0f,
              0.14717723f,   0.96535170f,  -0.21548802f, 0.0f,
             -0.54445839f,   0.26095250f,   0.79716301f, 0.0f,
            -70.73913600f, 110.45105000f, 361.73785000f, 1.0f
        );
        mpCamera->SetParentMatrix( cameraMatrix );

        int width, height;
        CPUTOSServices::GetOSServices()->GetClientDimensions(&width, &height);
        mpCamera->SetAspectRatio(((float)width)/((float)height));
        mpCamera->SetFov(XMConvertToRadians(90.0f)); // TODO: Fix converter's FOV bug (Maya generates cameras for which fbx reports garbage for fov)
        mpCamera->SetNearPlaneDistance(1.0f);
        mpCamera->SetFarPlaneDistance(8000.0f);
        mpCamera->Update();
    }

    // Set up the shadow camera (a camera that sees what the light sees)
    float3 lookAtPoint(0.0f, 0.0f, 0.0f);
    float3 half(1.0f, 1.0f, 1.0f);
    if( mpTerrainSet )
    {
        mpTerrainSet->GetBoundingBox( &lookAtPoint, &half );
    }
    float length = half.length();

    mpShadowCamera = new CPUTCamera();
    mpShadowCamera->SetFov(XMConvertToRadians(10));
    mpShadowCamera->SetAspectRatio( 4.0f );
    float fov = mpShadowCamera->GetFov();
    float tanHalfFov = tanf(fov * 0.5f);
    float cameraDistance = length/tanHalfFov;
    float nearDistance = cameraDistance * 0.1f;
    mpShadowCamera->SetNearPlaneDistance(nearDistance);
    mpShadowCamera->SetFarPlaneDistance(2.0f * cameraDistance);
    CPUTAssetLibraryDX11::GetAssetLibrary()->AddCamera( _L("ShadowCamera"), mpShadowCamera );
    float3 shadowCameraPosition = lookAtPoint - gLightDir * cameraDistance;
    mpShadowCamera->SetPosition( shadowCameraPosition );
    mpShadowCamera->LookAt( lookAtPoint.x, lookAtPoint.y, lookAtPoint.z );
    mpShadowCamera->Update();
#if GENERATE_BILLBOARD_TEXTURES
    // Note: We render to a square (aspectRatio=1).  Account for that if need to use non-square.
    // TODO: Get camera distance from .fol file.  I think setting it to the 3d cull distance should work.
    cameraDistance = gBillboardDistance;
    float fovY = 2.0f * atanf(half.y/(cameraDistance - half.z)); 
    float fovX = 2.0f * atanf(half.x/(cameraDistance - half.z));
    fov = max(fovX, fovY);
    mpCamera->SetFov(fov);
    mpCamera->SetFarPlaneDistance(2.0f * cameraDistance); // Could be more precise, but just need it to be well behind the plant we're rendering.
    mpCamera->SetAspectRatio(1.0f);
    float3 cameraPosition = lookAtPoint - float3(0.0f, 0.0f, cameraDistance);
    mpCamera->SetPosition( cameraPosition );
    mpCamera->LookAt( lookAtPoint.x, lookAtPoint.y, lookAtPoint.z );
    mpCamera->Update();
#endif // GENERATE_BILLBOARD_TEXTURES

    mpCameraController = new CPUTCameraControllerFPS();
    mpCameraController->SetCamera(mpCamera);
    mpCameraController->SetLookSpeed(0.004f);
    mpCameraController->SetMoveSpeed(5.0f);

    mpShadowCameraController = new CPUTCameraControllerArcBall();
    mpShadowCameraController->SetCamera(mpShadowCamera);
    mpShadowCameraController->SetLookSpeed(0.004f);
}

//-----------------------------------------------------------------------------
void MySample::Create()
{
    int windowWidth, windowHeight;
    CPUTOSServices::GetOSServices()->GetClientDimensions(&windowWidth, &windowHeight);

    CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();

    gLightDir.normalize();
    mAmbientColor = float3(0.12f, 0.20f, 0.30f);
    mLightColor   = float3(1.00f, 0.90f, 0.85f) * 1.2f;

    // TODO: read from cmd line, using these as defaults
    pAssetLibrary->SetMediaDirectoryName(    _L("Media\\"));

    CPUTGuiControllerDX11 *pGUI = CPUTGetGuiController();

    // Create some controls
    CPUTButton     *pButton = NULL;
    pGUI->CreateButton(_L("Fullscreen"), ID_FULLSCREEN_BUTTON, ID_MAIN_PANEL, &pButton);

    CPUTCheckbox   *pVsync = NULL;
    pGUI->CreateCheckbox(_L("VSYNC"),    ID_VSYNC_CHECKBOX,    ID_MAIN_PANEL, &pVsync);
    pVsync->SetCheckboxState( CPUT_CHECKBOX_CHECKED );

    // Create Static text
    pGUI->CreateText( _L("F1 for Help"),                                ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);
    pGUI->CreateText( _L("[Escape] to quit application"),               ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);
    pGUI->CreateText( _L("A,S,D,F - move camera position"),             ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);
    pGUI->CreateText( _L("Q - camera position up"),                     ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);
    pGUI->CreateText( _L("E - camera position down"),                   ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);
    pGUI->CreateText( _L("mouse + right click - camera look location"), ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);

    pGUI->SetActivePanel(ID_MAIN_PANEL);
    pGUI->DrawFPS(true);

    // Add our programatic (and global) material parameters
    CPUTMaterial::mGlobalProperties.AddValue( _L("cbPerFrameValues"), _L("$cbPerFrameValues") );
    CPUTMaterial::mGlobalProperties.AddValue( _L("cbPerModelValues"), _L("$cbPerModelValues") );
    CPUTMaterial::mGlobalProperties.AddValue( _L("_Shadow"), _L("$shadow_depth") );

    // Create default shaders
    CPUTPixelShaderDX11  *pPS       = CPUTPixelShaderDX11::CreatePixelShaderFromMemory(            _L("$DefaultShader"), CPUT_DX11::mpD3dDevice,          _L("PSMain"), _L("ps_4_0"), gpDefaultShaderSource );
    CPUTPixelShaderDX11  *pPSNoTex  = CPUTPixelShaderDX11::CreatePixelShaderFromMemory(   _L("$DefaultShaderNoTexture"), CPUT_DX11::mpD3dDevice, _L("PSMainNoTexture"), _L("ps_4_0"), gpDefaultShaderSource );
    CPUTVertexShaderDX11 *pVS       = CPUTVertexShaderDX11::CreateVertexShaderFromMemory(          _L("$DefaultShader"), CPUT_DX11::mpD3dDevice,          _L("VSMain"), _L("vs_4_0"), gpDefaultShaderSource );
    CPUTVertexShaderDX11 *pVSNoTex  = CPUTVertexShaderDX11::CreateVertexShaderFromMemory( _L("$DefaultShaderNoTexture"), CPUT_DX11::mpD3dDevice, _L("VSMainNoTexture"), _L("vs_4_0"), gpDefaultShaderSource );

    // We just want to create them, which adds them to the library.  We don't need them any more so release them, leaving refCount at 1 (only library owns a ref)
    SAFE_RELEASE(pPS);
    SAFE_RELEASE(pPSNoTex);
    SAFE_RELEASE(pVS);
    SAFE_RELEASE(pVSNoTex);

    // load shadow casting material+sprite object
    cString ExecutableDirectory;
    CPUTOSServices::GetOSServices()->GetExecutableDirectory(&ExecutableDirectory);
    pAssetLibrary->SetMediaDirectoryName(  ExecutableDirectory+_L(".\\Media\\"));

    mpShadowRenderTarget = new CPUTRenderTargetDepth();
    mpShadowRenderTarget->CreateRenderTarget( cString(_L("$shadow_depth")), SHADOW_WIDTH_HEIGHT, SHADOW_WIDTH_HEIGHT, DXGI_FORMAT_D32_FLOAT );

    mpTerrainLightMapRenderTarget = new CPUTRenderTargetColor();
    mpTerrainLightMapRenderTarget->CreateRenderTarget( cString(_L("$TerrainLightMap")), TERRAIN_LIGHT_MAP_WIDTH_HEIGHT, TERRAIN_LIGHT_MAP_WIDTH_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB );

    const UINT MSAA_COUNT = 1; // Set larger to enable MSAA
    mpMSAABackBuffer = new CPUTRenderTargetColor();
    mpMSAABackBuffer->CreateRenderTarget( cString(_L("$BackBufferRT")), windowWidth, windowHeight, DXGI_FORMAT_R11G11B10_FLOAT, MSAA_COUNT );

    mpMSAADepthBuffer = new CPUTRenderTargetDepth();
    mpMSAADepthBuffer->CreateRenderTarget( cString(_L("$DepthBufferRT")), windowWidth, windowHeight, DXGI_FORMAT_D32_FLOAT, MSAA_COUNT );

    mpPostProcess = new CPUTPostProcess();
    mpPostProcess->CreatePostProcess(mpMSAABackBuffer);

#if GENERATE_BILLBOARD_TEXTURES
    mpBillboardRenderTargetColor = new CPUTRenderTargetColor();
    mpBillboardRenderTargetColor->CreateRenderTarget( cString(_L("$billboardColor")), BILLBOARD_WIDTH_HEIGHT, BILLBOARD_WIDTH_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB );

    mpBillboardRenderTargetNormal = new CPUTRenderTargetColor();
    mpBillboardRenderTargetNormal->CreateRenderTarget( cString(_L("$billboardNormal")), BILLBOARD_WIDTH_HEIGHT, BILLBOARD_WIDTH_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM ); // Normals aren't colors.  Encode linearly instead of sRGB

    mpBillboardRenderTargetDepth = new CPUTRenderTargetDepth();
    mpBillboardRenderTargetDepth->CreateRenderTarget( cString(_L("$billboardDepth")), BILLBOARD_WIDTH_HEIGHT, BILLBOARD_WIDTH_HEIGHT, DXGI_FORMAT_D32_FLOAT );
#endif

    // Call ResizeWindow() because it creates some resources that our materials might need (e.g., the back buffer)
    ResizeWindow(windowWidth, windowHeight);

    // ***************************
    // Override default sampler desc for our default shadowing sampler
    CPUTRenderStateBlockDX11 *pBlock = new CPUTRenderStateBlockDX11();
    CPUTRenderStateDX11 *pStates = pBlock->GetState();
    pStates->SamplerDesc[1].Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    pStates->SamplerDesc[1].AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
    pStates->SamplerDesc[1].AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
    pStates->SamplerDesc[1].ComparisonFunc = D3D11_COMPARISON_GREATER;
    pBlock->CreateNativeResources();
    CPUTAssetLibrary::GetAssetLibrary()->AddRenderStateBlock( _L("$DefaultRenderStates"), pBlock );
    pBlock->Release(); // We're done with it.  The library owns it now.

    // ***************************
#if GENERATE_BILLBOARD_TEXTURES
    pAssetLibrary->SetMediaDirectoryName(    _L("Media\\Foliage\\") );
    cString baseName = gFilename.substr(0, gFilename.length() - 4); // Strip .mtl extension
    mpTerrainSet = pAssetLibrary->GetAssetSet( gFilename, false );
#else

    CPUTMaterial *pMaterial = pAssetLibrary->GetMaterial( _L("ShadowCast") );
    SAFE_RELEASE(pMaterial); // Just want to load it so it will be in the asset library

    pMaterial = pAssetLibrary->GetMaterial( _L("AmbientShadowCast") );
    SAFE_RELEASE(pMaterial); // Just want to load it so it will be in the asset library

    pMaterial = pAssetLibrary->GetMaterial( _L("HeightField") );
    SAFE_RELEASE(pMaterial); // Just want to load it so it will be in the asset library

    pMaterial = pAssetLibrary->GetMaterial( _L("BoundingBox") );
    SAFE_RELEASE(pMaterial); // Just want to load it so it will be in the asset library

    pMaterial = pAssetLibrary->GetMaterial( _L("TerrainLightMap") );
    SAFE_RELEASE(pMaterial); // Just want to load it so it will be in the asset library

    pMaterial = pAssetLibrary->GetMaterial( _L("TerrainAndTreesLightMap") );
    SAFE_RELEASE(pMaterial); // Just want to load it so it will be in the asset library

    // Load our terrain and sky
    pAssetLibrary->SetMediaDirectoryName(      _L("Media\\Terrain\\") );
    mpTerrainSet = pAssetLibrary->GetAssetSet( _L("RollingHills.set") );

    // Uncomment the following two lines to include the sky
    // pAssetLibrary->SetMediaDirectoryName(      _L("Media\\Sky\\") );
    // mpSkySet = pAssetLibrary->GetAssetSet(     _L("Sky.set") );

    // ***************************
    // Render the terrain to our height field texture.
    // TODO: How much of this memory can we reclaim after this step?
    // Can theoretically just release.  But, AssetLibrary holds references too.
    // ***************************
    CPUTRenderParametersDX renderParams( mpContext );
    renderParams.mpCamera = mpCamera; // Note: rendering to height field doesn't use this camera; it directly performs orthographic math
    if( mpTerrainSet )
    {

#if BAKE_TERRAIN_TEXTURE
        CPUTRenderTargetColor *pRenderTarget;

        const float pClearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
        pRenderTarget = new CPUTRenderTargetColor();
        pRenderTarget->CreateRenderTarget( cString(_L("$HeightField")), BAKED_TEXTURE_WIDTH_HEIGHT, BAKED_TEXTURE_WIDTH_HEIGHT, DXGI_FORMAT_R16G16B16A16_FLOAT );

        pRenderTarget->SetRenderTarget( renderParams, NULL, 0, pClearColor, true );

        mAmbientColor = float3(1.0f, 1.0f, 1.0f);
        mLightColor = float3(0.0f, 0.0f, 0.0f);
        UpdatePerFrameConstantBuffer(0.0f);

        mpTerrainSet->RenderRecursive(renderParams, 1);
        pRenderTarget->RestoreRenderTarget( renderParams );
        pRenderTarget->SaveRenderTargetToFile( renderParams, _L("TerrainLayers.dds"), D3DX11_IFF_DDS );
exit(0);
#else
        const float pClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        CPUTRenderTargetColor *pTerrainHeightFieldRenderTarget = new CPUTRenderTargetColor();
        pTerrainHeightFieldRenderTarget->CreateRenderTarget( cString(_L("$HeightField")), HEIGHT_FIELD_WIDTH_HEIGHT, HEIGHT_FIELD_WIDTH_HEIGHT, DXGI_FORMAT_R16G16_UNORM );

        pTerrainHeightFieldRenderTarget->SetRenderTarget( renderParams, NULL, 0, pClearColor, true );
        mpTerrainSet->RenderRecursive(renderParams, CPUT_MATERIAL_INDEX_HEIGHT_FIELD);
        pTerrainHeightFieldRenderTarget->RestoreRenderTarget( renderParams );

        // Uncomment this line to save the results to a file
        // pTerrainHeightFieldRenderTarget->SaveRenderTargetToFile( renderParams, _L("TerrainHeight.dds"), D3DX11_IFF_DDS );

        SAFE_DELETE(pTerrainHeightFieldRenderTarget);
#endif // !BAKE_TERRAIN_TEXTURE
    }
    // Load the foliage
    pAssetLibrary->SetMediaDirectoryName( _L("Media\\Foliage\\") );
    mpFoliagePatch = new CPUTFoliagePatchDX11();
    mpFoliagePatch->LoadFoliagePatch( _L("TreesAndGrass.fol"), renderParams );
#endif // !GENERATE_BILLBOARD_TEXTURES
    CreateCameras();
}

//-----------------------------------------------------------------------------
void MySample::Update(double deltaSeconds)
{
    mpCameraController->Update((float)deltaSeconds);
    mpShadowCameraController->Update((float)deltaSeconds);
}

//-----------------------------------------------------------------------------
CPUTEventHandledCode MySample::HandleKeyboardEvent(CPUTKey key)
{
    static bool panelToggle = false;
    CPUTEventHandledCode    handled = CPUT_EVENT_UNHANDLED;
    CPUTGuiControllerDX11*  pGUI = CPUTGetGuiController();

    switch(key)
    {
    case KEY_F1:
        panelToggle = !panelToggle;
        if(panelToggle)
        {
            pGUI->SetActivePanel(ID_SECONDARY_PANEL);
        }
        else
        {
            pGUI->SetActivePanel(ID_MAIN_PANEL);
        }
        handled = CPUT_EVENT_HANDLED;
        break;
    case KEY_B:
        {
            gShowBoundingBoxes = !gShowBoundingBoxes;
        }
        handled = CPUT_EVENT_HANDLED;
        break;
    case KEY_C:
        {
            gCheckerBoard = (gCheckerBoard+1)%3;
        }
        handled = CPUT_EVENT_HANDLED;
        break;
    case KEY_L:
        {
            gEnableHDR = !gEnableHDR;
        }
        handled = CPUT_EVENT_HANDLED;
        break;
    case KEY_ESCAPE:
        handled = CPUT_EVENT_HANDLED;
        Shutdown();
        break;
    }

    // pass it to the camera controller
    if(handled == CPUT_EVENT_UNHANDLED)
    {
        handled = mpCameraController->HandleKeyboardEvent(key);
    }
    if(handled == CPUT_EVENT_UNHANDLED)
    {
        handled = mpShadowCameraController->HandleKeyboardEvent(key);
    }
    return handled;
}

//-----------------------------------------------------------------------------
CPUTEventHandledCode MySample::HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state)
{
    CPUTEventHandledCode handled = CPUT_EVENT_UNHANDLED;
    if( mpCameraController )
    {
        handled = mpCameraController->HandleMouseEvent(x, y, wheel, state);
        if( handled == !CPUT_EVENT_UNHANDLED )
        {
            return handled;
        }
    }
    if( mpShadowCameraController )
    {
        handled = mpShadowCameraController->HandleMouseEvent(x, y, wheel, state);
        if( handled == CPUT_EVENT_UNHANDLED)
        {
            return handled;
        }
        gUpdateShadows = true;
    }
    return handled;
}

//-----------------------------------------------------------------------------
void MySample::HandleCallbackEvent( CPUTEventID Event, CPUTControlID ControlID, CPUTControl *pControl )
{
    UNREFERENCED_PARAMETER(Event);
    CPUTCheckboxState checkboxState;
    switch(ControlID)
    {
    case ID_FULLSCREEN_BUTTON:
        CPUTToggleFullScreenMode();
        break;
    case ID_VSYNC_CHECKBOX:
        checkboxState = ((CPUTCheckbox*)pControl)->GetCheckboxState();
        mSyncInterval = (checkboxState == CPUT_CHECKBOX_CHECKED);
        break;
    default:
        break;
    }
}

//-----------------------------------------------------------------------------
void MySample::ResizeWindow(UINT width, UINT height)
{
    CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();

    // Before we can resize the swap chain, we must release any references to it.
    // We could have a "AssetLibrary::ReleaseSwapChainResources(), or similar.  But,
    // Generic "release all" works, is simpler to implement/maintain, and is not performance critical.
    pAssetLibrary->ReleaseTexturesAndBuffers();

    // Resize the CPUT-provided render target
    CPUT_DX11::ResizeWindow( width, height );

    // Resize our render targets to match the new resolution
    // Note: this isn't required.  The render target can be a different resolution than the window.
    mpMSAABackBuffer->RecreateRenderTarget(  width, height );
    mpMSAADepthBuffer->RecreateRenderTarget( width, height );
    mpPostProcess->RecreatePostProcess();

    if( mpCamera ) mpCamera->SetAspectRatio(((float)width)/((float)height));

    pAssetLibrary->RebindTexturesAndBuffers();
}

static ID3D11UnorderedAccessView *gpNullUAVs[CPUT_MATERIAL_MAX_TEXTURE_SLOTS] = {0};
//-----------------------------------------------------------------------------
void MySample::Render(double deltaSeconds)
{
    static int frameIndex = 1;
    CPUTRenderParametersDX renderParams( mpContext, frameIndex++ );

#define RENDER_SHADOWS 1
#if RENDER_SHADOWS && !GENERATE_BILLBOARD_TEXTURES
    //*******************************
    // Draw the shadow scene when required (e.g., when the light moves)
    //*******************************
    if( gUpdateShadows )
    {
        mpShadowCamera->Update();
        renderParams.mpCamera = mpShadowCamera;

        if( mpTerrainSet )
        {
            // Render terrain to shadow map
            mpShadowRenderTarget->SetRenderTarget( renderParams, 0, 0.0f, true );
            mpTerrainSet->RenderRecursive( renderParams, CPUT_MATERIAL_INDEX_SHADOW_CAST );
            mpShadowRenderTarget->RestoreRenderTarget(renderParams);

            // Receive shadows on top-down view of terrain (i.e. generate a light map for the foliage to receive terrain shadows)
            float white[] = {1,1,1,1};
            mpTerrainLightMapRenderTarget->SetRenderTarget( renderParams, NULL, 0, white, true );
            mpTerrainSet->RenderRecursive( renderParams, CPUT_MATERIAL_INDEX_TERRAIN_LIGHT_MAP );
            mpTerrainLightMapRenderTarget->RestoreRenderTarget( renderParams );
        }
        if( mpFoliagePatch )
        {
            // Render the foliage to the shadow map.  TODO: Could render "terrain + foliage" light map here.
            mpShadowRenderTarget->SetRenderTarget( renderParams );
            mpFoliagePatch->RenderShadows(renderParams, CPUT_MATERIAL_INDEX_SHADOW_CAST);
            mpShadowRenderTarget->RestoreRenderTarget(renderParams);

            // Receive terrain and tree shadows to a light map
            mpTerrainLightMapRenderTarget->SetRenderTarget( renderParams );
            if( mpTerrainSet )
            {
                mpTerrainSet->RenderRecursive( renderParams, CPUT_MATERIAL_INDEX_TERRAIN_AND_TREES_LIGHT_MAP );
            }
            mpFoliagePatch->RenderShadows(renderParams, CPUT_MATERIAL_INDEX_AMBIENT_SHADOW_CAST);
            mpTerrainLightMapRenderTarget->RestoreRenderTarget( renderParams );
        }
        // Don't update again next time (unless someone else sets this flag again)
        gUpdateShadows = false;
    }
#endif // RENDER_SHADOWS

    renderParams.mpCamera = mpCamera;

    // Save the light direction to a global so we can set it to a constant buffer later (TODO: have light do this)
    gLightDir = gLightDir.normalize();
    gLightDir = mpShadowCamera->GetLook();

    renderParams.mShowBoundingBoxes = gShowBoundingBoxes;

#if GENERATE_BILLBOARD_TEXTURES
    const float pClearColorAlbedo[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float pClearColorNormal[]  = { 0.0f, 0.0f, 0.0f, 0.0f }; // { 0.5f, 0.5f, 1.0f, 0.0f };
    const float *pClearColorList[] = {
        pClearColorAlbedo,
        pClearColorNormal
    };
    // TODO: We overload mpTerrainSet here.  Don't.  Use a different set for this to aboid confusion.
    if( mpTerrainSet )
    {
        mpCamera->SetAspectRatio(1.0f);
        mpCamera->Update();

        mpBillboardRenderTargetColor->SetRenderTarget( renderParams, mpBillboardRenderTargetDepth, 0, pClearColorList[0], true );
        mpTerrainSet->RenderRecursive(renderParams, -1);
        mpBillboardRenderTargetColor->RestoreRenderTarget( renderParams );
        mpBillboardRenderTargetColor->SaveRenderTargetToFile( renderParams, _L("billboardColor.bmp"), D3DX11_IFF_BMP );

        mpBillboardRenderTargetNormal->SetRenderTarget( renderParams, mpBillboardRenderTargetDepth, 0, pClearColorList[1], true );
        mpTerrainSet->RenderRecursive(renderParams, -2);
        mpBillboardRenderTargetNormal->RestoreRenderTarget( renderParams );
        mpBillboardRenderTargetNormal->SaveRenderTargetToFile( renderParams, _L("billboardNormal.bmp"), D3DX11_IFF_BMP );

        exit(0); // We're done.
    }
    const float *pClearColor = pClearColorList[1];
#else
    const float pClearColor[] = { 0.2f, 0.3f, 0.4f, 1.0f };
#endif // !GENERATE_BILLBOARD_TEXTURES

    if( gEnableHDR )
    {
        mpMSAABackBuffer->SetRenderTarget( renderParams, mpMSAADepthBuffer, 0, pClearColor, true, 0.0f );
    } else
    {
        // Clear back buffer.
        mpContext->ClearRenderTargetView( mpBackBufferRTV,  pClearColor );
        mpContext->ClearDepthStencilView( mpDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);
    }
    if( mpTerrainSet )   { mpTerrainSet->RenderRecursive(renderParams); }
    if( mpSkySet )       { mpSkySet->RenderRecursive(renderParams); }
    if( mpFoliagePatch ) { mpFoliagePatch->Render(renderParams); }

    if( gEnableHDR )
    {
        mpMSAABackBuffer->RestoreRenderTarget( renderParams );
        mpPostProcess->PerformPostProcess( renderParams );
    }
    CPUTDrawGUI();
}
