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
#include "CPUTModelDX11.h"
#include "CPUTMaterialDX11.h"
#include "CPUTRenderParamsDX.h"
#include "CPUTFrustum.h"
#include "CPUTTextureDX11.h"
#include "CPUTBufferDX11.h"

#define GENERATE_BILLBOARD_TEXTURES 0

ID3D11Buffer *CPUTModelDX11::mpModelConstantBuffer = NULL;

// Return the mesh at the given index (cast to the GFX api version of CPUTMeshDX11)
//-----------------------------------------------------------------------------
CPUTMeshDX11* CPUTModelDX11::GetMesh(const UINT index) const
{
    return ( 0==mMeshCount || index > mMeshCount) ? NULL : (CPUTMeshDX11*)mpMesh[index];
}

float3 gLightDir( -0.5, -0.3f, 0.9f ); // Note: needs to be normalized

// Set the render state before drawing this object
//-----------------------------------------------------------------------------
void CPUTModelDX11::UpdateShaderConstants(CPUTRenderParameters &renderParams)
{
    ID3D11DeviceContext *pContext  = ((CPUTRenderParametersDX*)&renderParams)->mpContext;
    D3D11_MAPPED_SUBRESOURCE mapInfo;
    pContext->Map( mpModelConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapInfo );
    {
        CPUTModelConstantBuffer *pCb = (CPUTModelConstantBuffer*)mapInfo.pData;

        // TODO: remove construction of XMM type
        XMMATRIX     world((float*)GetWorldMatrix());
        pCb->World = world;

        CPUTCamera *pCamera   = renderParams.mpCamera;
        XMVECTOR    cameraPos = XMLoadFloat3(&XMFLOAT3( 0.0f, 0.0f, 0.0f ));
        if( pCamera )
        {
            XMMATRIX    view((float*)pCamera->GetViewMatrix());
            XMMATRIX    projection((float*)pCamera->GetProjectionMatrix());
            float      *pCameraPos = (float*)&pCamera->GetPosition();
            cameraPos = XMLoadFloat3(&XMFLOAT3( pCameraPos[0], pCameraPos[1], pCameraPos[2] ));

            // Note: We compute viewProjection to a local to avoid reading from write-combined memory.
            // The constant buffer uses write-combined memory.  We read this matrix when computing WorldViewProjection.
            // It is very slow to read it directly from the constant buffer.
            XMMATRIX viewProjection  = view * projection;
            pCb->ViewProjection      = viewProjection;
            pCb->WorldViewProjection = world * viewProjection;
            XMVECTOR determinant     = XMMatrixDeterminant(world);
            pCb->InverseWorld        = XMMatrixInverse(&determinant, XMMatrixTranspose(world));
        }
        // TODO: Have the lights set their render states?

        XMVECTOR lightDirection = XMLoadFloat3(&XMFLOAT3( gLightDir.x, gLightDir.y, gLightDir.z ));
        pCb->LightDirection     = XMVector3Normalize(lightDirection);
        pCb->EyePosition        = cameraPos;
        float *bbCWS = (float*)&mBoundingBoxCenterWorldSpace;
        float *bbHWS = (float*)&mBoundingBoxHalfWorldSpace;
        float *bbCOS = (float*)&mBoundingBoxCenterObjectSpace;
        float *bbHOS = (float*)&mBoundingBoxHalfObjectSpace;
        pCb->BoundingBoxCenterWorldSpace  = XMLoadFloat3(&XMFLOAT3( bbCWS[0], bbCWS[1], bbCWS[2] ));
        pCb->BoundingBoxHalfWorldSpace    = XMLoadFloat3(&XMFLOAT3( bbHWS[0], bbHWS[1], bbHWS[2] ));
        pCb->BoundingBoxCenterObjectSpace = XMLoadFloat3(&XMFLOAT3( bbCOS[0], bbCOS[1], bbCOS[2] ));
        pCb->BoundingBoxHalfObjectSpace   = XMLoadFloat3(&XMFLOAT3( bbHOS[0], bbHOS[1], bbHOS[2] ));

        // Shadow camera
        XMMATRIX    shadowView, shadowProjection;
        CPUTCamera *pShadowCamera = gpSample->GetShadowCamera();
        if( pShadowCamera )
        {
            shadowView = XMMATRIX((float*)pShadowCamera->GetViewMatrix());
            shadowProjection = XMMATRIX((float*)pShadowCamera->GetProjectionMatrix());
            pCb->LightWorldViewProjection = world * shadowView * shadowProjection;
        }
    }
    pContext->Unmap(mpModelConstantBuffer,0);
}

// Render this model. Render only this model, not its children or siblings.
//-----------------------------------------------------------------------------
void CPUTModelDX11::Render(CPUTRenderParameters &renderParams, int materialIndex)
{
    CPUTRenderParametersDX *pParams = (CPUTRenderParametersDX*)&renderParams;
    CPUTCamera             *pCamera = pParams->mpCamera;

    // TODO: Move bboxDirty to member and set only when model moves.
    bool bboxDirty = true;
    if( bboxDirty )
    {
        UpdateBoundsWorldSpace();
    }

#ifdef SUPPORT_DRAWING_BOUNDING_BOXES 
    if( renderParams.mShowBoundingBoxes && (!pCamera || pCamera->mFrustum.IsVisible( mBoundingBoxCenterWorldSpace, mBoundingBoxHalfWorldSpace )))
    {
        DrawBoundingBox( renderParams );
    }
#endif
    if( !renderParams.mDrawModels ) { return; }

    if( !pParams->mRenderOnlyVisibleModels || !pCamera || pCamera->mFrustum.IsVisible( mBoundingBoxCenterWorldSpace, mBoundingBoxHalfWorldSpace ) )
    {
        // Update the model's constant buffer.
        // Note that the materials reference this, so we need to do it only once for all of the model's meshes.
        UpdateShaderConstants(renderParams);

        // loop over all meshes in this model and draw them
        for(UINT ii=0; ii<mMeshCount; ii++)
        {
            UINT finalMaterialIndex = GetMaterialIndex(ii, materialIndex);
            ASSERT( finalMaterialIndex < mpSubMaterialCount[ii], _L("material index out of range."));
            CPUTMaterialDX11 *pMaterial = (CPUTMaterialDX11*)(mpMaterial[ii][finalMaterialIndex]);
            pMaterial->SetRenderStates(renderParams);

            ((CPUTMeshDX11*)mpMesh[ii])->Draw(renderParams, mpInputLayout[ii][finalMaterialIndex]);
        }
    }
}

//-----------------------------------------------------------------------------
void CPUTModelDX11::CreateModelConstantBuffer()
{
    CPUTAssetLibraryDX11 *pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();

    // Create the model constant buffer.
    HRESULT hr;
    D3D11_BUFFER_DESC bd = {0};
    bd.ByteWidth = sizeof(CPUTModelConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = (CPUT_DX11::GetDevice())->CreateBuffer( &bd, NULL, &mpModelConstantBuffer );
    ASSERT( !FAILED( hr ), _L("Error creating constant buffer.") );
    CPUTSetDebugName( mpModelConstantBuffer, _L("Model Constant buffer") );
    cString name = _L("$cbPerModelValues");
    CPUTBufferDX11 *pBuffer = new CPUTBufferDX11(name, mpModelConstantBuffer);
    pAssetLibrary->AddConstantBuffer( name, pBuffer );
    pBuffer->Release(); // We're done with it.  We added it to the library.  Release our reference.
}

// Load the set file definition of this object
// 1. Parse the block of name/parent/transform info for model block
// 2. Load the model's binary payload (i.e., the meshes)
// 3. Assert the # of meshes matches # of materials
// 4. Load each mesh's material
//-----------------------------------------------------------------------------
CPUTResult CPUTModelDX11::LoadModel(CPUTConfigBlock *pBlock, int *pParentID, CPUTModel *pMasterModel, int numSystemMaterials, cString *pSystemMaterialNames)
{
    CPUTResult result = CPUT_SUCCESS;
    CPUTAssetLibraryDX11 *pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();

    cString modelSuffix = ptoc(this);

    // set the model's name
    mName = pBlock->GetValueByName(_L("name"))->ValueAsString();
    mName = mName + _L(".mdl");

    // resolve the full path name
    cString modelLocation;
    cString resolvedPathAndFile;
    modelLocation = ((CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary())->GetModelDirectoryName();
    modelLocation = modelLocation+mName;
    CPUTOSServices::GetOSServices()->ResolveAbsolutePathAndFilename(modelLocation, &resolvedPathAndFile);	

    // Get the parent ID.  Note: the caller will use this to set the parent.
    *pParentID = pBlock->GetValueByName(_L("parent"))->ValueAsInt();

    LoadParentMatrixFromParameterBlock( pBlock );

    // Get the bounding box information
    float3 center(0.0f), half(0.0f);
    pBlock->GetValueByName(_L("BoundingBoxCenter"))->ValueAsFloatArray(center.f, 3);
    pBlock->GetValueByName(_L("BoundingBoxHalf"))->ValueAsFloatArray(half.f, 3);
    mBoundingBoxCenterObjectSpace = center;
    mBoundingBoxHalfObjectSpace   = half;

    mMeshCount = pBlock->GetValueByName(_L("meshcount"))->ValueAsInt();
    mpMesh     = new CPUTMesh*[mMeshCount];
    mpSubMaterialCount = new UINT[mMeshCount];
    mpMaterial = new CPUTMaterial**[mMeshCount];
    memset( mpMaterial, 0, mMeshCount * sizeof(CPUTMaterial*) );
    mpInputLayout = new ID3D11InputLayout**[mMeshCount];
    memset( mpInputLayout, 0, mMeshCount * sizeof(ID3D11InputLayout**) );
    
    cString materialName;
    char pNumber[4];
    cString materialValueName;

    CPUTModelDX11 *pMasterModelDX = (CPUTModelDX11*)pMasterModel;

    for(UINT ii=0; ii<mMeshCount; ii++)
    {
        if(pMasterModelDX)
        {
            // Reference the master model's mesh.  Don't create a new one.
            mpMesh[ii] = pMasterModelDX->mpMesh[ii];
            mpMesh[ii]->AddRef();
        }
        else
        {
            mpMesh[ii] = new CPUTMeshDX11();
        }
    }
    if( !pMasterModelDX )
    {
        // Not a clone/instance.  So, load the model's binary payload (i.e., vertex and index buffers)
        // TODO: Change to use GetModel()
        result = LoadModelPayload(resolvedPathAndFile);
        ASSERT( CPUTSUCCESS(result), _L("Failed loading model") );
    }

    for(UINT ii=0; ii<mMeshCount; ii++)
    {
        // get the right material number ('material0', 'material1', 'material2', etc)
        materialValueName = _L("material");
        _itoa_s(ii, pNumber, 4, 10);
        materialValueName.append(s2ws(pNumber));
        materialName = pBlock->GetValueByName(materialValueName)->ValueAsString();

        // Get/load material for this mesh
        cString meshSuffix  = itoc(ii);
#if GENERATE_BILLBOARD_TEXTURES
        cString *pFinalSystemNames = new cString[numSystemMaterials+2];
        pFinalSystemNames[1] = materialName + _L("Color");
        pFinalSystemNames[0] = materialName + _L("ColorNormal");
        int finalNumSystemMaterials = 2;
#else
        UINT totalNameCount = numSystemMaterials + NUM_GLOBAL_SYSTEM_MATERIALS;
        cString *pFinalSystemNames = new cString[totalNameCount];

        // Copy "global" system materials to caller-supplied list
        for( int jj=0; jj<numSystemMaterials; jj++ )
        {
            pFinalSystemNames[jj] = pSystemMaterialNames[jj];
        }
        pFinalSystemNames[totalNameCount + CPUT_MATERIAL_INDEX_SHADOW_CAST]                 = _L("..\\..\\Material\\ShadowCast");
        pFinalSystemNames[totalNameCount + CPUT_MATERIAL_INDEX_AMBIENT_SHADOW_CAST]         = _L("..\\..\\Material\\AmbientShadowCast");
        pFinalSystemNames[totalNameCount + CPUT_MATERIAL_INDEX_HEIGHT_FIELD]                = _L("..\\..\\Material\\HeightField");
        pFinalSystemNames[totalNameCount + CPUT_MATERIAL_INDEX_BOUNDING_BOX]                = _L("..\\..\\Material\\BoundingBox");
        pFinalSystemNames[totalNameCount + CPUT_MATERIAL_INDEX_TERRAIN_LIGHT_MAP]           = _L("..\\..\\Material\\TerrainLightMap");
        pFinalSystemNames[totalNameCount + CPUT_MATERIAL_INDEX_TERRAIN_AND_TREES_LIGHT_MAP] = _L("..\\..\\Material\\TerrainAndTreesLightMap");

        int finalNumSystemMaterials = numSystemMaterials + NUM_GLOBAL_SYSTEM_MATERIALS;
#endif
        CPUTMaterialDX11 *pMaterial = (CPUTMaterialDX11*)pAssetLibrary->GetMaterial(materialName, false, modelSuffix, meshSuffix, NULL, finalNumSystemMaterials, pFinalSystemNames);
        ASSERT( pMaterial, _L("Couldn't find material.") );

        delete []pFinalSystemNames;

        mpSubMaterialCount[ii] = pMaterial->GetSubMaterialCount();
        HEAPCHECK;
        SetSubMaterials(ii, pMaterial->GetSubMaterials());
        HEAPCHECK;

        // Release the extra refcount we're holding from the GetMaterial operation earlier
        // now the asset library, and this model have the only refcounts on that material
        pMaterial->Release();
    }

    return result;
}

//-----------------------------------------------------------------------------
void CPUTModelDX11::SetSubMaterials(UINT ii, CPUTMaterial **pMaterial)
{
    CPUTModel::SetSubMaterials(ii, pMaterial);
    HEAPCHECK;

    // Can't bind the layout if we haven't loaded the mesh yet.
    CPUTMeshDX11 *pMesh = (CPUTMeshDX11*)mpMesh[ii];
    D3D11_INPUT_ELEMENT_DESC *pDesc = pMesh->GetLayoutDescription();

    // Release the old input layouts and delete the layout array.
    if( mpInputLayout[ii] )
    {
        for( UINT jj=0; jj<mpSubMaterialCount[ii]; jj++ )
        {
            SAFE_RELEASE(mpInputLayout[ii][jj]);
        }
        SAFE_DELETE(mpInputLayout[ii]);
    }

    mpInputLayout[ii] = new ID3D11InputLayout*[mpSubMaterialCount[ii]];
    memset( mpInputLayout[ii], 0, sizeof(ID3D11InputLayout*) * mpSubMaterialCount[ii] );
    for( UINT jj=0; jj<mpSubMaterialCount[ii]; jj++ )
    {
        if( pDesc )
        {
            pMesh->BindVertexShaderLayout((CPUTMaterialDX11*)mpMaterial[ii][jj], &mpInputLayout[ii][jj]);
            HEAPCHECK;
        }
    }
}

#ifdef SUPPORT_DRAWING_BOUNDING_BOXES
//-----------------------------------------------------------------------------
void CPUTModelDX11::DrawBoundingBox(CPUTRenderParameters &renderParams)
{
    UpdateShaderConstants(renderParams);

    UINT index  = mpSubMaterialCount[0] + CPUT_MATERIAL_INDEX_BOUNDING_BOX;
    CPUTMaterialDX11 *pMaterial = (CPUTMaterialDX11*)(mpMaterial[0][index]);
    pMaterial->SetRenderStates(renderParams);

    ((CPUTMeshDX11*)mpBoundingBoxMesh)->Draw(renderParams, mpInputLayout[0][index]);
}

// Note that we only need one of these.  We don't need to re-create it for every model.
//-----------------------------------------------------------------------------
void CPUTModelDX11::CreateBoundingBoxMesh()
{
    CPUTResult result = CPUT_SUCCESS;

    float3 pVertices[8] = {
        float3(  1.0f,  1.0f,  1.0f ), // 0
        float3(  1.0f,  1.0f, -1.0f ), // 1
        float3( -1.0f,  1.0f,  1.0f ), // 2
        float3( -1.0f,  1.0f, -1.0f ), // 3
        float3(  1.0f, -1.0f,  1.0f ), // 4
        float3(  1.0f, -1.0f, -1.0f ), // 5
        float3( -1.0f, -1.0f,  1.0f ), // 6
        float3( -1.0f, -1.0f, -1.0f )  // 7
    };
    USHORT pIndices[24] = {
       0,1,  1,3,  3,2,  2,0,  // Top
       4,5,  5,7,  7,6,  6,4,  // Bottom
       0,4,  1,5,  2,6,  3,7   // Verticals
    };
    CPUTVertexElementDesc pVertexElements[] = {
        { CPUT_VERTEX_ELEMENT_POSITON, tFLOAT, 12, 0 },
    };

    CPUTMesh *pMesh = mpBoundingBoxMesh = new CPUTMeshDX11();
    pMesh->SetMeshTopology(CPUT_TOPOLOGY_INDEXED_LINE_LIST);

    CPUTBufferInfo vertexElementInfo;
    vertexElementInfo.mpSemanticName         = "POSITION";
    vertexElementInfo.mSemanticIndex         = 0;
    vertexElementInfo.mElementType           = CPUT_F32;
    vertexElementInfo.mElementComponentCount = 3;
    vertexElementInfo.mElementSizeInBytes    = 12;
    vertexElementInfo.mElementCount          = 8;
    vertexElementInfo.mOffset                = 0;

    CPUTBufferInfo indexDataInfo;
    indexDataInfo.mElementType           = CPUT_U16;
    indexDataInfo.mElementComponentCount = 1;
    indexDataInfo.mElementSizeInBytes    = sizeof(UINT16);
    indexDataInfo.mElementCount          = 24; // 12 lines, 2 verts each
    indexDataInfo.mOffset                = 0;
    indexDataInfo.mSemanticIndex         = 0;
    indexDataInfo.mpSemanticName         = NULL;

    result = pMesh->CreateNativeResources(
        this, -1,
        1,                    // vertexFormatDesc.mFormatDescriptorCount,
        &vertexElementInfo,
        pVertices,            // (void*)vertexFormatDesc.mpVertices,
        &indexDataInfo,
        pIndices              // &vertexFormatDesc.mpIndices[0]
    );
    ASSERT( CPUTSUCCESS(result), _L("Failed building bounding box mesh") );

    cString modelSuffix = ptoc(this);
    UINT index  = mpSubMaterialCount[0] + CPUT_MATERIAL_INDEX_BOUNDING_BOX;
    CPUTMaterialDX11 *pMaterial = (CPUTMaterialDX11*)(mpMaterial[0][index]);
    ((CPUTMeshDX11*)pMesh)->BindVertexShaderLayout( pMaterial, &mpInputLayout[0][index] );
}
#endif

