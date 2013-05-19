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
#include "CPUTFoliagePatch.h"
#include "CPUTAssetLibraryDX11.h"
#include "CPUTBufferDX11.h"
#include "CPUTTextureDX11.h"

//-----------------------------------------------------------------------------
float *CPUTFoliagePatchMeshDX11::mpRandomValues = NULL;

extern float3 gLightDir;

//-----------------------------------------------------------------------------
CPUTResult CPUTFoliagePatchDX11::LoadFoliagePatchParameters( const cString &name, CPUTRenderParametersDX &params )
{
    CPUTAssetLibraryDX11 *pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();
    CPUTConfigFile ConfigFile;
    CPUTConfigBlock *pBlock;
    CPUTConfigEntry *pEntry;
    CPUTResult result = ConfigFile.LoadFile(pAssetLibrary->GetMediaDirectoryName() + name);
    ASSERT(CPUTSUCCESS(result), _L("Error loading ") + name );

    mName = name;

    pBlock = ConfigFile.GetBlockByName(_L("Foliage_DX11"));
    ASSERT( pBlock, _L("Can't find parameter block [Foliage_DX11]") );
    if( !pBlock )
    {
        return CPUT_ERROR_PARAMETER_BLOCK_NOT_FOUND;
    }

    // TODO: Set the parent matrix?  Or, use world positions
    //       note: bbox is still accurate, so no culling motivation to use local space
    //       Using local space would allow lower-precision x,y positions

    // TODO: Determine bounding box from all the plants
    pEntry = pBlock->GetValueByName(_L("widthInPlants"));
    mWidthInPlants = pEntry->IsValid() ? pEntry->ValueAsInt() : 128;

    pEntry = pBlock->GetValueByName(_L("heightInPlants"));
    mHeightInPlants = pEntry->IsValid() ? pEntry->ValueAsInt() : 128;

    pEntry = pBlock->GetValueByName(_L("widthInPatches"));
    mWidthInPatches = pEntry->IsValid() ? pEntry->ValueAsInt() : 128;

    pEntry = pBlock->GetValueByName(_L("heightInPatches"));
    mHeightInPatches = pEntry->IsValid() ? pEntry->ValueAsInt() : 128;

    pEntry = pBlock->GetValueByName(_L("heightScale"));
    mHeightScale = pEntry->IsValid() ? pEntry->ValueAsFloat() : 600.0f;

    pEntry = pBlock->GetValueByName(_L("heightOffset"));
    mHeightOffset = pEntry->IsValid() ? pEntry->ValueAsFloat() : 0.0f;

    pEntry = pBlock->GetValueByName(_L("heightTextureWorldWidth"));
    mHeightTextureWorldWidth = pEntry->IsValid() ? pEntry->ValueAsFloat() : 5285.28f;

    pEntry = pBlock->GetValueByName(_L("heightTextureWorldHeight"));
    mHeightTextureWorldHeight = pEntry->IsValid() ? pEntry->ValueAsFloat() : 5285.28f;

    pEntry = pBlock->GetValueByName(_L("worldWidth"));
    mWorldWidth = pEntry->IsValid() ? pEntry->ValueAsFloat() : 5273.76f;

    pEntry = pBlock->GetValueByName(_L("worldHeight"));
    mWorldHeight = pEntry->IsValid() ? pEntry->ValueAsFloat() : 5273.76f;

    mUScale =  1.0f/mWorldWidth;
    mVScale =  1.0f/mWorldHeight;

    mHeightTextureName = pBlock->GetValueByName(_L("HeightTexture"))->ValueAsString();
    mMaterialName      = pBlock->GetValueByName(_L("material"))->ValueAsString();

    int blockCount = ConfigFile.BlockCount(); // Will have at most this many plants.
    mpFoliagePlantParameters     = new CPUTFoliagePlantParameters[blockCount];
    mpFilenameColorList          = new cString[blockCount];
    mpFilenameColorPtrList       = new cString*[blockCount];
    mpFilenameNormalList         = new cString[blockCount];
    mpFilenameNormalPtrList      = new cString*[blockCount];
    mpFilenameProbabilityList    = new cString[blockCount];
    mpFilenameProbabilityPtrList = new cString*[blockCount];
    mp3DPlantAssetSet            = new CPUTAssetSetDX11*[blockCount];
    mpProbabilityTexels          = new UCHAR*[blockCount];

    mNumPlantTypes = 0;
    int ii;
    for( ii=0; ii<blockCount; ii++ )
    {
        pBlock = ConfigFile.GetBlockByName(_L("plant ")+ itoc(ii+1));
        // TODO: Load the following from the file instead of hard-coding here.
        if( !pBlock )
        {
            // Reached the end.
            break;
        }
        mNumPlantTypes++;

        pEntry = pBlock->GetValueByName(_L("assetSet"));
        mp3DPlantAssetSet[ii] = pEntry->IsValid() ? (CPUTAssetSetDX11*)pAssetLibrary->GetAssetSet( pEntry->ValueAsString() ) : NULL;

        cString undefined = _L("$Undefined");
        mpFilenameColorPtrList[ii] = &mpFilenameColorList[ii];
        pEntry = pBlock->GetValueByName(_L("billboardColorTexture"));
        cString billboardColorTextureName = pEntry->IsValid() ? (pAssetLibrary->GetTextureDirectoryName() + pEntry->ValueAsString()) : undefined;
        mpFilenameColorList[ii] =billboardColorTextureName;

        mpFilenameNormalPtrList[ii] = &mpFilenameNormalList[ii];
        pEntry = pBlock->GetValueByName(_L("billboardNormalTexture"));
        cString billboardNormalTextureName = pEntry->IsValid() ? (pAssetLibrary->GetTextureDirectoryName() + pEntry->ValueAsString()) : undefined;
        mpFilenameNormalList[ii] = billboardNormalTextureName;

        mpFilenameProbabilityPtrList[ii] = &mpFilenameProbabilityList[ii];
        pEntry = pBlock->GetValueByName(_L("probabilityTexture"));
        cString probabilityTextureName = pEntry->IsValid() ? (pAssetLibrary->GetTextureDirectoryName() + pEntry->ValueAsString()) : undefined;
        mpFilenameProbabilityList[ii] = probabilityTextureName;

        pEntry = pBlock->GetValueByName(_L("3DFarClipDistance"));
        mpFoliagePlantParameters[ii].m3DFarClipDistance = pEntry->IsValid() ? pEntry->ValueAsFloat() : 10.0f;

        pEntry = pBlock->GetValueByName(_L("sizeMin"));
        mpFoliagePlantParameters[ii].mSizeMin = pEntry->IsValid() ? pEntry->ValueAsFloat() : 2.0f;

        pEntry = pBlock->GetValueByName(_L("sizeMax"));
        mpFoliagePlantParameters[ii].mSizeMax = pEntry->IsValid() ? pEntry->ValueAsFloat() : 3.0f;

        pEntry = pBlock->GetValueByName(_L("widthRatioMin"));
        mpFoliagePlantParameters[ii].mWidthRatioMin = pEntry->IsValid() ? pEntry->ValueAsFloat() : 0.5f;

        pEntry = pBlock->GetValueByName(_L("WidthRatioMax"));
        mpFoliagePlantParameters[ii].mWidthRatioMax = pEntry->IsValid() ? pEntry->ValueAsFloat() : 1.0f;

        pEntry = pBlock->GetValueByName(_L("plantSpacingWidth"));
        mpFoliagePlantParameters[ii].mPlantSpacingWidth = pEntry->IsValid() ? pEntry->ValueAsFloat() : 2.5f;

        pEntry = pBlock->GetValueByName(_L("plantSpacingHeight"));
        mpFoliagePlantParameters[ii].mPlantSpacingHeight = pEntry->IsValid() ? pEntry->ValueAsFloat() : 2.5f;

        pEntry = pBlock->GetValueByName(_L("positionRandomScale"));
        mpFoliagePlantParameters[ii].mPositionRandScale = pEntry->IsValid() ? pEntry->ValueAsFloat() : 0.5f;

        pEntry = pBlock->GetValueByName(_L("ULeft"));
        mpFoliagePlantParameters[ii].mULeft = pEntry->IsValid() ? pEntry->ValueAsFloat() : 0.0f;

        pEntry = pBlock->GetValueByName(_L("URight"));
        mpFoliagePlantParameters[ii].mURight = pEntry->IsValid() ? pEntry->ValueAsFloat() : 1.0f;

        pEntry = pBlock->GetValueByName(_L("VTop"));
        mpFoliagePlantParameters[ii].mVTop = pEntry->IsValid() ? pEntry->ValueAsFloat() : 1.0f;

        pEntry = pBlock->GetValueByName(_L("VBottom"));
        mpFoliagePlantParameters[ii].mVBottom = pEntry->IsValid() ? pEntry->ValueAsFloat() : 0.0f;

        pEntry = pBlock->GetValueByName(_L("OffsetX"));
        mpFoliagePlantParameters[ii].mOffsetX = pEntry->IsValid() ? pEntry->ValueAsFloat() : 0.0f;

        pEntry = pBlock->GetValueByName(_L("OffsetY"));
        mpFoliagePlantParameters[ii].mOffsetY = pEntry->IsValid() ? pEntry->ValueAsFloat() : 0.0f;

        pEntry = pBlock->GetValueByName(_L("NearFadeStart"));
        mpFoliagePlantParameters[ii].mNearFadeStart = pEntry->IsValid() ? pEntry->ValueAsFloat() : 0.0f;

        pEntry = pBlock->GetValueByName(_L("NearFadeEnd"));
        mpFoliagePlantParameters[ii].mNearFadeEnd = pEntry->IsValid() ? pEntry->ValueAsFloat() : -1.0f;

        pEntry = pBlock->GetValueByName(_L("FarFadeStart"));
        mpFoliagePlantParameters[ii].mFarFadeStart = pEntry->IsValid() ? pEntry->ValueAsFloat() : 0.0f;

        pEntry = pBlock->GetValueByName(_L("FarFadeEnd"));
        mpFoliagePlantParameters[ii].mFarFadeEnd = pEntry->IsValid() ? pEntry->ValueAsFloat() : -1.0f;

        mpFoliagePlantParameters[ii].mTotalWidth         =  mWidthInPlants  * mpFoliagePlantParameters[ii].mPlantSpacingWidth;
        mpFoliagePlantParameters[ii].mTotalHeight        =  mHeightInPlants * mpFoliagePlantParameters[ii].mPlantSpacingHeight;
        mpFoliagePlantParameters[ii].mOneOverTotalWidth  =  1.0f/mpFoliagePlantParameters[ii].mTotalWidth;
        mpFoliagePlantParameters[ii].mOneOverTotalHeight =  1.0f/mpFoliagePlantParameters[ii].mTotalHeight;
    }
    // NULL-terminate the name lists
    mpFilenameColorPtrList[ii] = NULL;
    mpFilenameNormalPtrList[ii] = NULL;
    mpFilenameProbabilityPtrList[ii] = NULL;

    return result;
}

//-----------------------------------------------------------------------------
CPUTResult CPUTFoliagePatchDX11::LoadFoliagePatch( const cString &name, CPUTRenderParametersDX &params )
{
    // Load the paramters from the foliage config file.
    CPUTResult result = LoadFoliagePatchParameters( name, params );
    ASSERT( CPUTSUCCESS(result), _L("Failed loading foliage patch parameters") );

    CPUTAssetLibraryDX11 *pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();

    // ************************
    // Load the texture arrays.
    // Form each of our array textures from multiple textures
    // (i.e., store as simple 2D textures on disk and create array textures at load)
    // ************************
    cString textureArrayName;

    textureArrayName= _L("$FoliageBillboardColor");
    mpFoliageBillboardTextureArrayColor = CPUTTexture::CreateTextureArrayFromFilenameList( params, textureArrayName, mpFilenameColorPtrList, true );
    if( mpFoliageBillboardTextureArrayColor )
    {
        pAssetLibrary->AddTexture( textureArrayName, mpFoliageBillboardTextureArrayColor );
    }

    textureArrayName = _L("$FoliageBillboardNormal");
    mpFoliageBillboardTextureArrayNormal = CPUTTexture::CreateTextureArrayFromFilenameList( params, textureArrayName, mpFilenameNormalPtrList, false ); // Note: Disabled sRGB for normal map
    if( mpFoliageBillboardTextureArrayNormal )
    {
        pAssetLibrary->AddTexture( textureArrayName, mpFoliageBillboardTextureArrayNormal );
    }

    textureArrayName = _L("$FoliageProbability");
    mpFoliageBillboardTextureArrayProbability = CPUTTexture::CreateTextureArrayFromFilenameList( params, textureArrayName, mpFilenameProbabilityPtrList, false ); // Note: Disabled sRGB for normal map
    if( mpFoliageBillboardTextureArrayProbability )
    {
        pAssetLibrary->AddTexture( textureArrayName, mpFoliageBillboardTextureArrayProbability );
    }

    // ************************
    // Create the patch
    // ************************
    UINT halfWidthInPatches  = mWidthInPatches/2;
    UINT halfHeightInPatches = mHeightInPatches/2;

    mMeshCount = mWidthInPatches * mHeightInPatches;
    mpMesh     = new CPUTMesh*[mMeshCount];
    mpMaterial = new CPUTMaterial**[mMeshCount];
    memset( mpMaterial, 0, mMeshCount * sizeof(CPUTMaterial**) );
    
    // Get pointer to height texels
    mpHeightTexture = pAssetLibrary->GetTexture( mHeightTextureName, false, false ); // Note: load as srgb = false
    D3D11_MAPPED_SUBRESOURCE subResource = mpHeightTexture->MapTexture( params, CPUT_MAP_READ );

    mpHeightTexels = (USHORT*)subResource.pData;
    mHeightTextureWidth  = HEIGHT_FIELD_WIDTH_HEIGHT; // TODO: Use real dimensions.
    mHeightTextureHeight = HEIGHT_FIELD_WIDTH_HEIGHT;

    mpProbabilityTexture       = new CPUTTexture*[mNumPlantTypes];
    mpProbabilityTextureMapped = new bool[mNumPlantTypes];
    for( int ii=0; ii<mNumPlantTypes; ii++ )
    {
        // Get pointer to probability texels
        if( mpFilenameProbabilityList[ii] != _L("$Undefined") )
        {
            mpProbabilityTexture[ii] = pAssetLibrary->GetTexture( mpFilenameProbabilityList[ii], true, false ); // Note: load as srgb = false
            // If we already have this texture, then use the original mapping.
            // This brute-force approach should be fine for the small numbers of plants we use.  If it proves to be expensive, there are faster ways to search a list.
            int jj;
            for( jj=0; jj<ii; jj++ )
            {
                if( mpProbabilityTexture[ii] == mpProbabilityTexture[jj] )
                {
                    // found it.  Use the existing mapping
                    mpProbabilityTexels[ii] = mpProbabilityTexels[jj];
                    break; 
                }
            }
            if( jj == ii )
            {
                D3D11_MAPPED_SUBRESOURCE subResource = mpProbabilityTexture[ii]->MapTexture( params, CPUT_MAP_READ );
                mpProbabilityTexels[ii] = (UCHAR*)subResource.pData;
                mpProbabilityTextureMapped[ii] = true;
            } else
            {
                mpProbabilityTextureMapped[ii] = false;
            }
        } else
        {
            mpProbabilityTexture[ii] = NULL;
            mpProbabilityTexels[ii] = NULL;
            mpProbabilityTextureMapped[ii] = false;
        }
    }
    // TODO: Assert that all are same dimensions
    mProbabilityTextureWidth  = 1024; // TODO: Use real dimensions.
    mProbabilityTextureHeight = 1024;

    UINT meshIdx = 0;
    for( int ii=0; ii<mWidthInPatches; ii++ )
    {
        for( int jj=0; jj<mHeightInPatches; jj++, meshIdx++ )
        {
            mpMesh[meshIdx] = new CPUTFoliagePatchMeshDX11();
            CPUTFoliagePatchMeshDX11 *pMesh = (CPUTFoliagePatchMeshDX11*)mpMesh[meshIdx];
            pMesh->CreateFoliagePatchMesh(
                this,
                meshIdx,
                mpHeightTexels,
                mpProbabilityTexels,
                mHeightTextureWidth,
                mHeightTextureHeight,
                mProbabilityTextureWidth,
                mProbabilityTextureHeight,
                jj-halfWidthInPatches,
                ii-halfHeightInPatches
            );
        }
    }
    mpHeightTexture->UnmapTexture( params );

    // ************************
    // Get/load material
    // ************************
    // TODO: Best way to convert float to string?  Then, remove casting to int below.
    char pTmp[64];
    cString modelSuffix         = ptoc(this);
    cString nearFadeStart       = itoc((int)mpFoliagePlantParameters[0].mNearFadeStart);
    cString nearFadeEnd         = itoc((int)mpFoliagePlantParameters[0].mNearFadeEnd);
    _snprintf_s( pTmp, 64, "%f", 1.0f/(mpFoliagePlantParameters[0].mNearFadeEnd - mpFoliagePlantParameters[0].mNearFadeStart) );
    cString nearFadeDenominator = s2ws( pTmp );
    cString farFadeStart        = itoc((int)mpFoliagePlantParameters[0].mFarFadeStart);
    cString farFadeEnd          = itoc((int)mpFoliagePlantParameters[0].mFarFadeEnd);
    _snprintf_s( pTmp, 64, "%f", 1.0f/(mpFoliagePlantParameters[0].mFarFadeEnd - mpFoliagePlantParameters[0].mFarFadeStart) );
    cString farFadeDenominator  = s2ws( pTmp );
    cString plantSpacingWidth   = itoc((int)mpFoliagePlantParameters[0].mPlantSpacingWidth);
    for( int ii=1; ii<mNumPlantTypes; ii++ )
    {
        nearFadeStart       += _L(",") + itoc((int)mpFoliagePlantParameters[ii].mNearFadeStart);
        nearFadeEnd         += _L(",") + itoc((int)mpFoliagePlantParameters[ii].mNearFadeEnd);
        _snprintf_s( pTmp, 64, ",%f", 1.0f/(mpFoliagePlantParameters[ii].mNearFadeEnd - mpFoliagePlantParameters[ii].mNearFadeStart) );
        nearFadeDenominator += s2ws( pTmp );

        farFadeStart        += _L(",") + itoc((int)mpFoliagePlantParameters[ii].mFarFadeStart);
        farFadeEnd          += _L(",") + itoc((int)mpFoliagePlantParameters[ii].mFarFadeEnd);
        _snprintf_s( pTmp, 64, ",%f", 1.0f/(mpFoliagePlantParameters[ii].mFarFadeEnd - mpFoliagePlantParameters[ii].mFarFadeStart) );
        farFadeDenominator  += s2ws( pTmp );
    }
    
    // Convert to char *.  Note: save pointers so we can delete them when we're done with them.
    char *pMaxPlantTypesChars       = ws2s(itoc(mNumPlantTypes));
    char *pNearFadeStartChars       = ws2s(nearFadeStart);
    char *pNearFadeEndChars         = ws2s(nearFadeEnd);
    char *pFarFadeStartChars        = ws2s(farFadeStart);
    char *pFarFadeEndChars          = ws2s(farFadeEnd);
    char *pPlantSpacingWidthChars   = ws2s(plantSpacingWidth);
    char *pFarFadeDenominatorChars  = ws2s(farFadeDenominator);
    char *pNearFadeDenominatorChars = ws2s(nearFadeDenominator);

    D3D_SHADER_MACRO pDefines[] = {
        "MAX_PLANT_TYPES",       pMaxPlantTypesChars,
        "NEAR_FADE_START",       pNearFadeStartChars,
        "NEAR_FADE_END",         pNearFadeEndChars,
        "FAR_FADE_START",        pFarFadeStartChars,
        "FAR_FADE_END",          pFarFadeEndChars,
        "FAR_FADE_DENOMINATOR",  pFarFadeDenominatorChars,
        "NEAR_FADE_DENOMINATOR", pNearFadeDenominatorChars,
        "PLANT_SPACING_WIDTH",   pPlantSpacingWidthChars,
        NULL, NULL
    };

    mpSubMaterialCount = new UINT[mMeshCount];
    mpInputLayout = new ID3D11InputLayout**[mMeshCount];
    memset( mpInputLayout, 0, mMeshCount * sizeof(ID3D11InputLayout**));
    for( UINT ii=0; ii<mMeshCount; ii++ )
    {
        CPUTMaterialDX11 *pMaterial = (CPUTMaterialDX11*)pAssetLibrary->GetMaterial(mMaterialName, false, modelSuffix, itoc(ii), (char**)pDefines);
        ASSERT( pMaterial, _L("Couldn't find material.") );

        mpSubMaterialCount[ii] = pMaterial->GetSubMaterialCount();
        SetSubMaterials( ii, pMaterial->GetSubMaterials() );

        // Release the extra refcount we're holding from the GetMaterial operation earlier
        // now the asset library, and this model have the only refcounts on that material
        pMaterial->Release();
    }
    // Cleanup our temporaries
    delete pMaxPlantTypesChars;
    delete pNearFadeStartChars;
    delete pNearFadeEndChars;
    delete pFarFadeStartChars;
    delete pFarFadeEndChars;
    delete pPlantSpacingWidthChars;
    delete pFarFadeDenominatorChars;
    delete pNearFadeDenominatorChars;

    int totalNumPlants = mHeightInPlants * mWidthInPlants;
    mpPlantLast3DFrameTime = new int*[mNumPlantTypes];
    mpPlantLast3DParentMatrix = new float4x4*[mNumPlantTypes];
    for( int ii=0; ii<mNumPlantTypes; ii++ )
    {
        mpPlantLast3DFrameTime[ii] = new int[totalNumPlants];
        memset(mpPlantLast3DFrameTime[ii], -1, sizeof(int)*totalNumPlants);

        mpPlantLast3DParentMatrix[ii] = new float4x4[totalNumPlants];
        memset(mpPlantLast3DParentMatrix[ii], 0, sizeof(float4x4)*totalNumPlants);
    }

    mpPatchSortNodes = new FoliagePatchSortNode[mWidthInPatches * mHeightInPatches];
    int index = 0;
    for( int yy = 0; yy<mHeightInPatches; yy++ )
    {
        for( int xx = 0; xx<mWidthInPatches; xx++, index++ )
        {
            mpPatchSortNodes[index].mIndex = index;
            mpPatchSortNodes[index].mpMesh = (CPUTFoliagePatchMeshDX11*)mpMesh[index];
        }
    }
    return result;
}

// Note:  We have two different, but very similar, texture-fetch functions.
// One fetches from a 1-short per-texel texture.
// The other fetches from a 4-byte per-texel texture.
// TODO: best way to simplify/combine?
// Also note that these functions are not highly-optimized.
// This  is OK, as they are currently called only at create-time.
// TODO: If needed at runtime, then consider alternatives (e.g., multiple fetches with SIMD, etc.)
//       at the least, would make sure to move as much work outside the inner fetch-loop as possible.
//-----------------------------------------------------------------------------
// TODO: Move to CPUTTexture
USHORT BilinearTextureFetchHeight(
    USHORT *pTexels,
    UINT    textureWidth,
    UINT    heightTextureHeight,
    float   uu,
    float   vv
){
    // Convert from normalized coords to texture coords
    uu = textureWidth        * uu        - 0.5f;
    vv = heightTextureHeight * (1.0f-vv) + 0.5f;

    uu = fmod(uu, (float)textureWidth );
    vv = fmod(vv, (float)heightTextureHeight );

    if( uu < 0.0f ) uu += textureWidth;
    if( vv < 0.0f ) vv += heightTextureHeight;

    USHORT texel00 = pTexels[2*(textureWidth * min(textureWidth-1, ((UINT)vv + 0)) + min(textureWidth-1, ((UINT)uu + 0)))];
    USHORT texel10 = pTexels[2*(textureWidth * min(textureWidth-1, ((UINT)vv + 0)) + min(textureWidth-1, ((UINT)uu + 1)))];
    USHORT texel01 = pTexels[2*(textureWidth * min(textureWidth-1, ((UINT)vv - 1)) + min(textureWidth-1, ((UINT)uu + 0)))];
    USHORT texel11 = pTexels[2*(textureWidth * min(textureWidth-1, ((UINT)vv - 1)) + min(textureWidth-1, ((UINT)uu + 1)))];

    float fracX  = uu - (UINT)uu;
    float fracY  = vv - (UINT)vv;

    float row0   = texel00 * (1.0f-fracX) + texel10 * fracX;
    float row1   = texel01 * (1.0f-fracX) + texel11 * fracX;

    float result = row1 * (1.0f-fracY) + row0 * fracY;
    return (USHORT)result;
}

//-----------------------------------------------------------------------------
// TODO: Move to CPUTTexture
float BilinearTextureFetchProbability(
    UCHAR  *pTexels,
    UINT    textureWidth,
    UINT    heightTextureHeight,
    float   uu,
    float   vv
){
    // Convert from normalized coords to texture coords
    uu = textureWidth  * uu               - 0.5f;
    vv = heightTextureHeight * (1.0f-vv)  + 0.5f;

    uu = fmod(uu, (float)textureWidth );
    vv = fmod(vv, (float)heightTextureHeight );

    if( uu < 0.0f ) uu += textureWidth;
    if( vv < 0.0f ) vv += heightTextureHeight;

    UCHAR texel00 = pTexels[4*(textureWidth * min(textureWidth-1, ((UINT)vv + 0)) + min(textureWidth-1, ((UINT)uu + 0)))];
    UCHAR texel10 = pTexels[4*(textureWidth * min(textureWidth-1, ((UINT)vv + 0)) + min(textureWidth-1, ((UINT)uu + 1)))];
    UCHAR texel01 = pTexels[4*(textureWidth * min(textureWidth-1, ((UINT)vv - 1)) + min(textureWidth-1, ((UINT)uu + 0)))];
    UCHAR texel11 = pTexels[4*(textureWidth * min(textureWidth-1, ((UINT)vv - 1)) + min(textureWidth-1, ((UINT)uu + 1)))];

    float fracX  = uu - (UINT)uu;
    float fracY  = vv - (UINT)vv;

    float row0   = texel00 * (1.0f-fracX) + texel10 * fracX;
    float row1   = texel01 * (1.0f-fracX) + texel11 * fracX;

    float result = row1 * (1.0f-fracY) + row0 * fracY;
    return result * 1.0f/255;
}

//-----------------------------------------------------------------------------
CPUTResult CPUTFoliagePatchMeshDX11::CreateFoliagePatchMesh(
    CPUTFoliagePatchDX11 *pFoliagePatch,
    UINT    meshIndex,
    USHORT *pHeightTexels,
    UCHAR **pProbabilityTexels,
    UINT    heightTextureWidth,
    UINT    heightTextureHeight,
    UINT    probabilityTextureWidth,
    UINT    probabilityTextureHeight,
    int     patchIndexX,
    int     patchIndexY
){
    // Patch size is dictated by the smallest plant type
    // TODO: How about with a quadtree?  Would include only plant types that could end up in the patch - e.g., small plants aren't present past their fade distance.
    // TODO: determine smallest plant that is visible in this patch.
    // The smallest plants will have faded out with distance
    // That allows us to have the patch cover more area (ala quadtree)
    float smallestPlantSpacingWidth   = pFoliagePatch->mpFoliagePlantParameters[0].mPlantSpacingWidth;
    float smallestPlantSpacingHeight  = pFoliagePatch->mpFoliagePlantParameters[0].mPlantSpacingHeight;

    float patchWidthInFeet  = pFoliagePatch->mWidthInPlants  * smallestPlantSpacingWidth;
    float patchHeightInFeet = pFoliagePatch->mHeightInPlants * smallestPlantSpacingHeight;

    float patchLeft   = patchIndexX * patchWidthInFeet;
    float patchTop    = patchIndexY * patchHeightInFeet;
    float patchRight  = patchLeft   + patchWidthInFeet;
    float patchBottom = patchTop    + patchHeightInFeet;

    int totalNumPlants = pFoliagePatch->mHeightInPlants * pFoliagePatch->mWidthInPlants;
	mNumPlantTypes = pFoliagePatch->mNumPlantTypes;

    // Identify the patch's bounds in the current plant's "space"
    // e.g. Given: patch spacing == 1 foot, plant spacing == 3 feet, and the patch width == 100 plants.
    //      then, patch bottom-left == 0,0.  top-right == 100,100.
    //      the larger plant also has bottom-left == 0,0
    //      but, top-right == 100/3,100/3 == 33.3, 33.3
    //      So, we loop for 0 to 33, computing each plant's placement
    ASSERT(totalNumPlants <= 0x0000FFFF, _L("Too many plants.")); //   Limiting to 64K total allows for 16-bit indices
    mpVertices = new CPUTFoliagePatchVertex[totalNumPlants];

    UINT  seed = 0x1234; // Pull a number out of a hat to set a known seed.  TODO: Allow artist to choose value (may allow them to work around some inconvenient randomness)
    srand( seed );

    // TODO: Do this only once, instead of repeating for every mesh.  Use a static member variable?
    int NUM_RANDOM_VALUES_PER_PLANT = 6;
    int numRandomValues = totalNumPlants * NUM_RANDOM_VALUES_PER_PLANT;
    if( !mpRandomValues )
    {
        mpRandomValues = new float[numRandomValues];
        float  oneOverMaxRand = 1.0f/(float)RAND_MAX;
        for( int ii=0; ii<numRandomValues; ii++ )
        {
            mpRandomValues[ii] = rand() * oneOverMaxRand;
        }
    }
    float3 minExtent(FLT_MAX), maxExtent(-FLT_MAX);

    for( int type=0; type<pFoliagePatch->mNumPlantTypes; type++ )
    {
        float positionRandScale  = pFoliagePatch->mpFoliagePlantParameters[type].mPositionRandScale;
        float plantSpacingWidth  = pFoliagePatch->mpFoliagePlantParameters[type].mPlantSpacingWidth;
        float plantSpacingHeight = pFoliagePatch->mpFoliagePlantParameters[type].mPlantSpacingHeight;

        float halfWidthInPlants      = float(pFoliagePatch->mWidthInPlants/2);
        float halfHeightInPlants     = float(pFoliagePatch->mHeightInPlants/2);
        float halfPlantSpacingWidth  = plantSpacingWidth*0.5f;
        float halfPlantSpacingHeight = plantSpacingHeight*0.5f;

        int left   = int(patchLeft  /plantSpacingWidth);
        int top    = int(patchTop   /plantSpacingHeight);

        float xoffset = left * plantSpacingWidth; // Start position of patch (in this plant's "space")
        float yoffset = top  * plantSpacingHeight;
        float patchXOffset = halfWidthInPlants  * plantSpacingWidth  + xoffset;
        float patchYOffset = halfHeightInPlants * plantSpacingHeight + yoffset;

        int widthInPlants  = int(patchRight /plantSpacingWidth)  - int(patchLeft/plantSpacingWidth);
        int heightInPlants = int(patchBottom/plantSpacingHeight) - int(patchTop /plantSpacingHeight);

        for( int ii=0; ii<heightInPlants; ii++ )
        {
            for( int jj=0; jj<widthInPlants; jj++ )
            {
                int randIndexX = int(patchXOffset/plantSpacingWidth + jj) % pFoliagePatch->mWidthInPlants;
                if( randIndexX < 0 ) { randIndexX += pFoliagePatch->mWidthInPlants; }
                int randIndexY = int(patchYOffset/plantSpacingHeight + ii) % pFoliagePatch->mHeightInPlants;
                if( randIndexY < 0 ) { randIndexY += pFoliagePatch->mHeightInPlants; }
                int randIndex = randIndexY * pFoliagePatch->mWidthInPlants + randIndexX;

                float randMinusOneToOne;
                randMinusOneToOne = mpRandomValues[randIndex+0] * 2.0f - 1.0f;
                float xOffset = patchXOffset + randMinusOneToOne * positionRandScale * plantSpacingWidth + halfPlantSpacingWidth;
                float xx = (jj-halfWidthInPlants) * plantSpacingWidth + xOffset;

                randMinusOneToOne = mpRandomValues[randIndex+1] * 2.0f - 1.0f;
                float yOffset = patchYOffset + randMinusOneToOne * positionRandScale * plantSpacingHeight + halfPlantSpacingHeight;
                float yy = (ii-halfHeightInPlants) * plantSpacingHeight + yOffset;

                float uu = xx * pFoliagePatch->mUScale + 0.5f;
                float vv = yy * pFoliagePatch->mVScale + 0.5f;
                USHORT heightTexture = BilinearTextureFetchHeight( pHeightTexels, heightTextureWidth, heightTextureHeight, uu, vv );
                float  probabilityTexture;
                if( pProbabilityTexels[type] )
                {
                    probabilityTexture = BilinearTextureFetchProbability( pProbabilityTexels[type], probabilityTextureWidth, probabilityTextureHeight, uu, vv );
                } else
                {
                    probabilityTexture = 1.0f;
                }
                float zz = pFoliagePatch->mHeightScale * ((float)heightTexture / (float)0xFFFF) + pFoliagePatch->mHeightOffset;

                // Where is this plant, in units of the smallest grid (where does it land in the smallest grid?)
                int gridIndexX = int((xx  - patchLeft)/smallestPlantSpacingWidth) % pFoliagePatch->mWidthInPlants;
                if( gridIndexX < 0 )
                {
                    gridIndexX += pFoliagePatch->mWidthInPlants;
                }
                int gridIndexY = int((yy  - patchTop)/smallestPlantSpacingHeight) % pFoliagePatch->mHeightInPlants;
                if( gridIndexY < 0 )
                {
                    gridIndexY += pFoliagePatch->mHeightInPlants;
                }
                int vertexIndex = gridIndexY*pFoliagePatch->mWidthInPlants+gridIndexX;
                ASSERT(vertexIndex < totalNumPlants, _L("Plant index out of range."));
                CPUTFoliagePatchVertex *pVertex = &mpVertices[vertexIndex];

                if( probabilityTexture < mpRandomValues[randIndex+2] )
                {
                    pVertex->mPlantTypeIndex = -1;
                    continue;
                }
                pVertex->mPlantTypeIndex = type;

                pVertex->mPosition = float3( xx, zz, yy );
                minExtent.x = min(minExtent.x, xx );
                minExtent.y = min(minExtent.y, zz ); // Swap Y and Z.  Our patch is in XY, but the ground plane is XZ in our world's coordinate system.
                minExtent.z = min(minExtent.z, yy );

                maxExtent.x = max(maxExtent.x, xx );
                maxExtent.y = max(maxExtent.y, zz ); // Swap Y and Z
                maxExtent.z = max(maxExtent.z, yy );

                CPUTFoliagePlantParameters *pThisPlantParameters = &pFoliagePatch->mpFoliagePlantParameters[pVertex->mPlantTypeIndex];
                float overallScaleRand   = mpRandomValues[randIndex+3] * (pThisPlantParameters->mSizeMax       - pThisPlantParameters->mSizeMin)       + pThisPlantParameters->mSizeMin;
                float xScaleRand         = mpRandomValues[randIndex+4] * (pThisPlantParameters->mWidthRatioMax - pThisPlantParameters->mWidthRatioMin) + pThisPlantParameters->mWidthRatioMin;
                pVertex->mScale          = float2( xScaleRand * overallScaleRand, overallScaleRand ); // float2( xScaleRand, overallScaleRand );

                pVertex->mUVExtent[0]    = pThisPlantParameters->mULeft;
                pVertex->mUVExtent[1]    = pThisPlantParameters->mURight;
                pVertex->mUVExtent[2]    = pThisPlantParameters->mVTop;
                pVertex->mUVExtent[3]    = pThisPlantParameters->mVBottom;

                pVertex->mOffset[0]      = pThisPlantParameters->mOffsetX;
                pVertex->mOffset[1]      = pThisPlantParameters->mOffsetY;
            }
        }
    }
    mBoundingBoxHalf   = 0.5f * (maxExtent - minExtent);
    mBoundingBoxCenter = minExtent + mBoundingBoxHalf;

    // We submit points - one for each plant. We use a geometry shader to expand to two triangles each plant.
    SetMeshTopology(CPUT_TOPOLOGY_POINT_LIST);

    // TODO: No need to do this work for every patch.  Every patch uses the same values.
    //       Move to caller.
    const UINT NUM_FOLIAGE_VERTEX_ELEMENTS = 5;
    CPUTBufferInfo pVertexElementInfo[NUM_FOLIAGE_VERTEX_ELEMENTS] = {
        { "POSITION", 0, CPUT_F32, 3,  12,  0, totalNumPlants },
        { "TEXCOORD", 0, CPUT_U32, 1,   4, 12, totalNumPlants },
        { "TEXCOORD", 1, CPUT_F32, 2,   8, 16, totalNumPlants },
        { "TEXCOORD", 2, CPUT_F32, 4,  16, 24, totalNumPlants },
        { "TEXCOORD", 3, CPUT_F32, 2,   8, 40, totalNumPlants }
    };

    CPUTResult result = CreateNativeResources(
        pFoliagePatch,
        meshIndex,
        NUM_FOLIAGE_VERTEX_ELEMENTS,
        pVertexElementInfo,
        mpVertices,
        NULL,
        NULL
    );
    mIndexCount = totalNumPlants;
    ASSERT( CPUTSUCCESS(result), _L("Failed building foliage mesh") );

    return result;
}

//-----------------------------------------------------------------------------
void CPUTFoliagePatchDX11::GetPlantPositionAndBbox(
    int     plantType,
    USHORT *pHeightTexels,
    UCHAR **pProbabilityTexels,
    UINT    heightTextureWidth,
    UINT    heightTextureHeight,
    UINT    probabilityTextureWidth,
    UINT    probabilityTextureHeight,
    int     gridIndexX,
    int     gridIndexY,
    float3 *pPlantPosition,
    float3 *pPlantBboxCenter,
    float3 *pPlantBboxHalf,
    bool   *pPlantActive
){
    CPUTFoliagePlantParameters *pPlantParams = &mpFoliagePlantParameters[plantType];
    float positionRandScale  = pPlantParams->mPositionRandScale;
    float plantSpacingWidth      = pPlantParams->mPlantSpacingWidth;
    float plantSpacingHeight     = pPlantParams->mPlantSpacingHeight;
    float halfPlantSpacingWidth  = plantSpacingWidth*0.5f;
    float halfPlantSpacingHeight = plantSpacingHeight*0.5f;

    // Get x,y offset in grid (how much is the plant jittered in it's grid box)
    // "Wrap" the indices (similar to texture-coordinate wrapping)
    int indexX = (gridIndexX-mWidthInPlants/2) % mWidthInPlants;
    if( indexX < 0 ) { indexX += mWidthInPlants; }
    int indexY = (gridIndexY-mHeightInPlants/2) % mHeightInPlants;
    if( indexY < 0 ) { indexY += mHeightInPlants; }

    int randIndex = indexY * mWidthInPlants + indexX;
    float randMinusOneToOne;
    randMinusOneToOne = CPUTFoliagePatchMeshDX11::mpRandomValues[randIndex+0] * 2.0f - 1.0f;
    float xOffset = randMinusOneToOne * positionRandScale * plantSpacingWidth + halfPlantSpacingWidth;
    pPlantPosition->x = gridIndexX * plantSpacingWidth + xOffset;

    randMinusOneToOne = CPUTFoliagePatchMeshDX11::mpRandomValues[randIndex+1] * 2.0f - 1.0f;
    float yOffset = randMinusOneToOne * positionRandScale * plantSpacingHeight + halfPlantSpacingHeight;
    pPlantPosition->z = gridIndexY * plantSpacingHeight + yOffset;

    float uu = pPlantPosition->x * mUScale + 0.5f;
    float vv = pPlantPosition->z * mVScale + 0.5f;
    USHORT heightTexture = BilinearTextureFetchHeight( pHeightTexels, heightTextureWidth, heightTextureHeight, uu, vv );
    pPlantPosition->y = mHeightScale * ((float)heightTexture / (float)0xFFFF) + mHeightOffset;

    //TODO: move texture fetch to CPUTTexture?
    float  probabilityTexture;
    if( pProbabilityTexels[plantType] )
    {
        probabilityTexture = BilinearTextureFetchProbability( pProbabilityTexels[plantType], probabilityTextureWidth, probabilityTextureHeight, uu, vv );
    } else
    {
        probabilityTexture = 1.0f;
    }
    *pPlantActive = ( probabilityTexture >= CPUTFoliagePatchMeshDX11::mpRandomValues[randIndex+2] );

    float overallScaleRand = CPUTFoliagePatchMeshDX11::mpRandomValues[randIndex+3] * (pPlantParams->mSizeMax       - pPlantParams->mSizeMin)       + pPlantParams->mSizeMin;
    float xScaleRand       = CPUTFoliagePatchMeshDX11::mpRandomValues[randIndex+4] * (pPlantParams->mWidthRatioMax - pPlantParams->mWidthRatioMin) + pPlantParams->mWidthRatioMin;
    float plantWidth       = xScaleRand * overallScaleRand;
    float plantHeight      = overallScaleRand;
    *pPlantBboxCenter      = float3(pPlantPosition->x, pPlantPosition->y + 0.5f*plantHeight, pPlantPosition->z );
    *pPlantBboxHalf        = float3(0.5f*plantWidth, 0.5f*plantHeight, 0.5f*plantWidth );
}

//-----------------------------------------------------------------------------
void CPUTFoliagePatchMeshDX11::Draw(CPUTRenderParameters &renderParams, ID3D11InputLayout *pInputLayout )
{
    ID3D11DeviceContext *pContext = ((CPUTRenderParametersDX*)&renderParams)->mpContext;

    // One point per plant.  Geometry shader expands to 4 verts == 2 triangles.
    pContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_POINTLIST );

    // Our Geometry Shader fetches from a buffer and doesn't use a traditional vertex buffer or index buffer.
    ID3D11Buffer *pEmptyVertexBufferList[] = {NULL};
    UINT          pNullUint[] = {NULL};
    pContext->IASetVertexBuffers(0, 1, pEmptyVertexBufferList, pNullUint, pNullUint);

    // DX runtime issues warning if we don't set an input layout.
    pContext->IASetInputLayout( pInputLayout );

    // Our sorting algorithm processes each index twice.
    // Once to select for post-sorted plants on the left side of the screen, and again for those on the right.
    // The sort order is different for left and right.  Converting between post-sorted indices is complex.
    // Simpler to process twice, and cull to early-out.  Also, allows ping-ponging between left and right.
    // e.g., if we always drew left first, then could cause sort errors at center of screen.
    pContext->Draw( 2*mIndexCount, 0 );
}

//-----------------------------------------------------------------------------
void CPUTFoliagePatchDX11::UpdateShaderConstants(CPUTRenderParameters &renderParams, const float3 &patchCenterPosition)
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
        CPUTCamera *pShadowCamera = renderParams.mpCamera;
        if( pShadowCamera )
        {
            shadowView = XMMATRIX((float*)pShadowCamera->GetViewMatrix());
            shadowProjection = XMMATRIX((float*)pShadowCamera->GetProjectionMatrix());
            pCb->LightWorldViewProjection = world * shadowView * shadowProjection;
        }
    }
    pContext->Unmap(mpModelConstantBuffer,0);
}

extern int gCheckerBoard;
//-----------------------------------------------------------------------------
void FoliagePatchSortNode::Render( CPUTRenderParameters &renderParams, CPUTFoliagePatchDX11 *pPatch )
{
    // Render any/all patches that are further from the camera
    if( mpRight ) mpRight->Render( renderParams, pPatch );

    int xx = mIndex % 8;
    int yy = mIndex / 8;
    if( !gCheckerBoard || ((xx+yy+gCheckerBoard)&1) )
    {
        pPatch->UpdateShaderConstants(renderParams, mpMesh->mBoundingBoxCenter);

        CPUTMaterialDX11 *pMaterial = (CPUTMaterialDX11*)(pPatch->mpMaterial[mIndex][0]);
        pMaterial->SetRenderStates(renderParams);

        mpMesh->Draw(renderParams, pPatch->mpInputLayout[mIndex][0]); // We don't need a unique input layout for every mesh.  We know they will all be the same.
    }
    // Render any/all patches that are closer to the camera
    if( mpLeft ) mpLeft->Render( renderParams, pPatch );
}

//-----------------------------------------------------------------------------
void CPUTFoliagePatchDX11::RenderBillboardPatches(CPUTRenderParameters &renderParams, int materialIndex )
{
    CPUTRenderParametersDX *pParams = (CPUTRenderParametersDX*)&renderParams;
    CPUTCamera             *pCamera = pParams->mpCamera;

    // **********************
    // Sort the patches wrt distance to eye. Note: patches are on XZ plane, use distance in XZ
    // **********************
    float patchWidth   = mWidthInPlants  * mpFoliagePlantParameters[0].mPlantSpacingWidth;
    float patchHeight  = mHeightInPlants * mpFoliagePlantParameters[0].mPlantSpacingHeight;
    float patchOffsetX = -(((float)mWidthInPatches  - 1) * 0.5f) * patchWidth;
    float patchOffsetZ = -(((float)mHeightInPatches - 1) * 0.5f) * patchHeight;
    float3 eyePosition  = pCamera->GetPosition();
    int index=0;
    mpPatchSortRootNode = NULL;
    for( int zz=0; zz<mHeightInPatches; zz++ )
    {
        for( int xx=0; xx<mWidthInPatches; xx++, index++ )
        {
            CPUTFoliagePatchMeshDX11 *pCurMesh = (CPUTFoliagePatchMeshDX11*)mpMesh[index];
            if( pCamera->mFrustum.IsVisible( pCurMesh->mBoundingBoxCenter, pCurMesh->mBoundingBoxHalf ) )
            {
                float3 patchCenter( patchOffsetX + xx*patchWidth, 0.0f, patchOffsetZ + zz*patchHeight ); // Note: dont' care about Y
                float3 eyeToCenter     = patchCenter - eyePosition;
                float  distanceSquared = eyeToCenter.x*eyeToCenter.x + eyeToCenter.z*eyeToCenter.z;
                FoliagePatchSortNode *pNode = &mpPatchSortNodes[index];
                pNode->Init( distanceSquared );

                if( mpPatchSortRootNode )
                {
                    mpPatchSortRootNode->Insert( pNode );
                } else
                {
                    mpPatchSortRootNode = pNode;
                }
            }
        }
    }
    // **********************
    // Render the patches in sorted order
    // **********************
    if( mpPatchSortRootNode )
    {
        mpPatchSortRootNode->Render( renderParams, this );
    }
}

//-----------------------------------------------------------------------------
void CPUTFoliagePatchDX11::Render(CPUTRenderParameters &renderParams, int materialIndex)
{
    CPUTRenderParametersDX *pParams = (CPUTRenderParametersDX*)&renderParams;
    CPUTCamera             *pCamera = pParams->mpCamera;

    if( !renderParams.mDrawModels ) { return; }

    ASSERT( pCamera, _L("You must supply a camera to render foliage."));

    float3 cameraPosition = pCamera->GetPosition();
    float tmp = cameraPosition.z;
    cameraPosition.z = cameraPosition.y;
    cameraPosition.y = tmp;

    float4x4 *pView = pCamera->GetViewMatrix();
    float3 look(pView->r0.z, pView->r1.z, pView->r2.z ); // _m02_m12_m22;

    // *****************************
    // Draw the visible 3D plants
    // TODO: Sort?
    float halfTotalWidth  = 0.5f * (mWidthInPatches  * mWidthInPlants  * mpFoliagePlantParameters[0].mPlantSpacingWidth);
    float halfTotalHeight = 0.5f * (mHeightInPatches * mHeightInPlants * mpFoliagePlantParameters[0].mPlantSpacingHeight);

    int num3DPlants = 0;
    for( int plantType=0; plantType<mNumPlantTypes; plantType++ )
    {
        CPUTAssetSetDX11 *pPlant3DAssetSet = mp3DPlantAssetSet[plantType];
        if( !pPlant3DAssetSet )
        {
            continue; // No 3D plant model for this plant type.
        }

        // Draw any of this patch's visible 3D plants
        // Walk the square around the camera
        float plant3DFarClipDistance = mpFoliagePlantParameters[plantType].m3DFarClipDistance;
        float plant3DFarClipDistanceSquared = plant3DFarClipDistance * plant3DFarClipDistance;

        int cameraGrid3DPlantX    = (int)(cameraPosition.x/mpFoliagePlantParameters[plantType].mPlantSpacingWidth);
        int cameraGrid3DPlantY    = (int)(cameraPosition.y/mpFoliagePlantParameters[plantType].mPlantSpacingHeight);
        int grid3DPlantHalfWidth  = (int)(mpFoliagePlantParameters[plantType].m3DFarClipDistance/mpFoliagePlantParameters[plantType].mPlantSpacingWidth);
        int grid3DPlantHalfHeight = (int)(mpFoliagePlantParameters[plantType].m3DFarClipDistance/mpFoliagePlantParameters[plantType].mPlantSpacingHeight);
        int grid3DLeft            = cameraGrid3DPlantX - grid3DPlantHalfWidth;
        int grid3DTop             = cameraGrid3DPlantY - grid3DPlantHalfHeight;
        int grid3DRight           = cameraGrid3DPlantX + grid3DPlantHalfWidth;
        int grid3DBottom          = cameraGrid3DPlantY + grid3DPlantHalfHeight;

        int rightMostIndex  = (int)(halfTotalWidth/mpFoliagePlantParameters[plantType].mPlantSpacingWidth);
        int leftMostIndex   = -rightMostIndex;
        int bottomMostIndex = (int)(halfTotalHeight/mpFoliagePlantParameters[plantType].mPlantSpacingHeight);
        int topMostIndex    = -bottomMostIndex;

        grid3DLeft   = max( leftMostIndex,   grid3DLeft );
        grid3DTop    = max( topMostIndex,    grid3DTop  );
        grid3DRight  = min( rightMostIndex,  grid3DRight );
        grid3DBottom = min( bottomMostIndex, grid3DBottom  );

        int width  = grid3DRight  - grid3DLeft;
        int height = grid3DBottom - grid3DTop;

        int gridY = grid3DTop;
        for( int ii=0; ii<height; ii++, gridY++ )
        {
            int gridX = grid3DLeft;
            for( int jj=0; jj<width; jj++, gridX++ )
            {
                // Compute this plant's position and bounding box
                float3 plantPosition, plantBboxCenter, plantBboxHalf;
                bool plantActive = false;
                GetPlantPositionAndBbox(
                    plantType,
                    mpHeightTexels,
                    mpProbabilityTexels,
                    mHeightTextureWidth,
                    mHeightTextureHeight,
                    mProbabilityTextureWidth,
                    mProbabilityTextureHeight,
                    gridX,
                    gridY,
                    &plantPosition,
                    &plantBboxCenter,
                    &plantBboxHalf,
                    &plantActive
                );
                if( !plantActive )
                {
                    // Plant didn't pass probability test
                    continue;
                }

                // If the plant isn't close enough to the camera, then skip it.
                // TODO: Resolve why sometimes y and z are flipped.  Likely because these models were exported from 3dsMax (and improper handling by FBXConvert)
                float3 cameraPositionXZY = float3( cameraPosition.x, cameraPosition.z, cameraPosition.y );
                float distanceSquaredEyeToPlant = (plantPosition - cameraPositionXZY).lengthSq();
                if( distanceSquaredEyeToPlant > plant3DFarClipDistanceSquared )
                {
                    continue; // Don't draw this plant as it is beyond the cull distance.
                }

                // If this plant is visible, then render it.
                if( pCamera->mFrustum.IsVisible( plantBboxCenter, plantBboxHalf ) )
                {
                    // Draw the tree.
                    num3DPlants++;
                    CPUTRenderNode *pRoot = pPlant3DAssetSet->GetRoot();

                    int indexX = (gridX-mWidthInPlants/2) % mWidthInPlants;
                    if( indexX < 0 ) { indexX += mWidthInPlants; }
                    int indexY = (gridY-mHeightInPlants/2) % mHeightInPlants;
                    if( indexY < 0 ) { indexY += mHeightInPlants; }

                    // If this plant wasn't visible last frame, then it is "newly visible"
                    int gridIndex = indexY*mWidthInPlants + indexX;
                    int lastFrameIndex = mpPlantLast3DFrameTime[plantType][gridIndex];
                    mpPlantLast3DFrameTime[plantType][gridIndex] = renderParams.mFrameIndex;

                    if( lastFrameIndex != (renderParams.mFrameIndex - 1) )
                    {
                        // Plant wasn't visible last frame, but is this frame.  Orient/align it to the billboard (==camera)
                        float3 look = pCamera->GetLook();
                        float3 up(0.0f, 1.0f, 0.0f);
                        float3 right = cross3(look, up).normalize();
                        look = cross3( up, right ).normalize();

                        mpPlantLast3DParentMatrix[plantType][gridIndex] = float4x4(
                            look.z, up.z, -right.z, 0.0f,
                            look.y, up.y, -right.y, 0.0f,
                            look.x, up.x, -right.x, 0.0f,
                            plantPosition.x, plantPosition.y, plantPosition.z, 1.0f
                        );
                        pRoot->SetParentMatrix( mpPlantLast3DParentMatrix[plantType][gridIndex] );
                    } else
                    {
                        pRoot->SetParentMatrix( mpPlantLast3DParentMatrix[plantType][gridIndex] );
                    }
                    pRoot->Update();
                    pPlant3DAssetSet->RenderRecursive( renderParams, materialIndex );
					pRoot->Release();
                }
            } // For each column of potential 3D plants
        } // For each row of potential 3D plants
    } // Foreacch plant type

    // Finished rendering the 3D plants.  Next, render the billboards.
    RenderBillboardPatches( renderParams, materialIndex );
}

//-----------------------------------------------------------------------------
void CPUTFoliagePatchDX11::RenderShadows(CPUTRenderParameters &renderParams, int materialIndex)
{
    CPUTRenderParametersDX *pParams = (CPUTRenderParametersDX*)&renderParams;
    CPUTCamera             *pCamera = pParams->mpCamera;

    if( !renderParams.mDrawModels ) { return; }

    ASSERT( pCamera, _L("You must supply a camera to render foliage."));

    float3 cameraPosition = pCamera->GetPosition();
    float tmp = cameraPosition.z;
    cameraPosition.z = cameraPosition.y;
    cameraPosition.y = tmp;

    // TODO: Align trees to light "look"
    float4x4 *pView = pCamera->GetViewMatrix();
    float3 look(pView->r0.z, pView->r1.z, pView->r2.z ); // _m02_m12_m22;

    // *****************************
    // Draw all 3D plants
    // TODO: Sorted
    // TODO: Use quadtree
    int   num3DPlants = 0;
    float halfTotalWidth  = 0.5f * (mWidthInPatches  * mWidthInPlants  * mpFoliagePlantParameters[0].mPlantSpacingWidth);
    float halfTotalHeight = 0.5f * (mHeightInPatches * mHeightInPlants * mpFoliagePlantParameters[0].mPlantSpacingHeight);

    for( int plantType=0; plantType<mNumPlantTypes; plantType++ )
    {
        CPUTAssetSetDX11 *pPlant3DAssetSet = mp3DPlantAssetSet[plantType];
        if( !pPlant3DAssetSet )
        {
            continue; // No 3D plant model for this plant type.
        }

        int right  = (int)(halfTotalWidth/mpFoliagePlantParameters[plantType].mPlantSpacingWidth);
        int left   = -right;
        int bottom = (int)(halfTotalHeight/mpFoliagePlantParameters[plantType].mPlantSpacingHeight);
        int top    = -bottom;

        for( int ii=left; ii<right; ii++ )
        {
            for( int jj=top; jj<bottom; jj++ )
            {
                // Compute this plant's position and bounding box
                float3 plantPosition, plantBboxCenter, plantBboxHalf;
                bool plantActive = false;
                GetPlantPositionAndBbox(
                    plantType,
                    mpHeightTexels,
                    mpProbabilityTexels,
                    mHeightTextureWidth,
                    mHeightTextureHeight,
                    mProbabilityTextureWidth,
                    mProbabilityTextureHeight,
                    jj,
                    ii,
                    &plantPosition,
                    &plantBboxCenter,
                    &plantBboxHalf,
                    &plantActive
                );
                if( !plantActive )
                {
                    // Plant didn't pass probability test
                    continue;
                }

                // Draw the plant.
                num3DPlants++;
                CPUTRenderNode *pRoot = pPlant3DAssetSet->GetRoot();

                int indexX = (jj-mWidthInPlants/2) % mWidthInPlants;
                if( indexX < 0 ) { indexX += mWidthInPlants; }
                int indexY = (ii-mHeightInPlants/2) % mHeightInPlants;
                if( indexY < 0 ) { indexY += mHeightInPlants; }

                int gridIndex = indexY*mWidthInPlants + indexX;

                float3 look = pCamera->GetLook();
                float3 up(0.0f, 1.0f, 0.0f);
                float3 right = cross3(look, up).normalize();
                look = cross3( up, right ).normalize();

                mpPlantLast3DParentMatrix[plantType][gridIndex] = float4x4(
                    look.z, up.z, -right.z, 0.0f,
                    look.y, up.y, -right.y, 0.0f,
                    look.x, up.x, -right.x, 0.0f,
                    plantPosition.x, plantPosition.y, plantPosition.z, 1.0f
                );
                pRoot->SetParentMatrix( mpPlantLast3DParentMatrix[plantType][gridIndex] );

                pRoot->Update();
                pPlant3DAssetSet->RenderRecursive( renderParams, materialIndex);
				pRoot->Release();
            } // For each column of potential 3D plants
        } // For each row of potential 3D plants
    } // Foreacch plant type
}

