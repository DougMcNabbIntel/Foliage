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
#ifndef __CPUTFOLIAGEPATCHDX11_H__
#define __CPUTFOLIAGEPATCHDX11_H__

#include "CPUTModelDX11.h"
class CPUTFoliagePatchDX11;

const UINT BILLBOARD_WIDTH_HEIGHT = 1024;
const UINT HEIGHT_FIELD_WIDTH_HEIGHT = 1024;
const UINT BAKED_TEXTURE_WIDTH_HEIGHT = 4096;

//-----------------------------------------------------------------------------
struct CPUTFoliagePatchVertex
{
    float3 mPosition;
    int    mPlantTypeIndex;
    float2 mScale;
    float  mUVExtent[4];
    float  mOffset[2];
};

//-----------------------------------------------------------------------------
class CPUTFoliagePatchMeshDX11 : public CPUTMeshDX11
{
    friend class CPUTFoliagePatchDX11;
    friend class FoliagePatchSortNode;

protected:
    static float *mpRandomValues;
    CPUTFoliagePatchVertex *mpVertices;
	int		mNumPlantTypes;

    // Other meshes don't have bounding boxes.  But, we need them.
    float3 mBoundingBoxCenter;
    float3 mBoundingBoxHalf;

    CPUTFoliagePatchMeshDX11 &operator=(CPUTFoliagePatchMeshDX11 &pOtherMesh) {ASSERT(0, _L("Assignment not supported")); return *this;} // Don't support assignment
    CPUTFoliagePatchMeshDX11( const CPUTFoliagePatchMeshDX11 &otherMesh ) {ASSERT(0, _L("Copy constructor not supported"));}

public:
    CPUTFoliagePatchMeshDX11() :
      mpVertices(NULL),
      mBoundingBoxCenter(0.0f),
	  mNumPlantTypes(0),
      mBoundingBoxHalf(0.0f)
    {
    }
    ~CPUTFoliagePatchMeshDX11()
    {
        SAFE_DELETE_ARRAY(mpVertices);
		SAFE_DELETE_ARRAY(mpRandomValues);
    }

    CPUTResult CreateFoliagePatchMesh(
        CPUTFoliagePatchDX11 *pFoliagePatch,
        UINT                  meshIndex,
        USHORT               *pHeightTexels,
        UCHAR               **pProbabilityTexels,
        UINT                  heightTextureWidth,
        UINT                  heightTextureHeight,
        UINT                  probabilityTextureWidth,
        UINT                  probabilityTextureHeight,
        int                   patchIndexX,
        int                   patchIndexY
    );
    void Draw( CPUTRenderParameters &renderParams );
    void Draw( CPUTRenderParameters &renderParams, ID3D11InputLayout *pInputLayout );
};

//-----------------------------------------------------------------------------
class CPUTFoliagePlantParameters
{
public:
    float    mSizeMin; // Shortest plant's height
    float    mSizeMax; // Tallest plant's height
    float    mWidthRatioMin; // Narrowest ratio (width-to-height) 
    float    mWidthRatioMax; // Widest ratio
    float    mPlantSpacingWidth;
    float    mPlantSpacingHeight;
    float    mPositionRandScale;
    float    mTotalWidth;
    float    mTotalHeight;
    float    mOneOverTotalWidth;
    float    mOneOverTotalHeight;
    float    mUScale;
    float    mVScale;
    float    mULeft;
    float    mURight;
    float    mVTop;
    float    mVBottom;
    float    mOffsetX;
    float    mOffsetY;
    float    mNearFadeStart;
    float    mNearFadeEnd;
    float    mFarFadeStart;
    float    mFarFadeEnd;
    float    m3DFarClipDistance;
};

//--------------------------------------------------------------------------------------
class FoliagePatchSortNode
{
public:
    int                       mIndex;
    float                     mDistance;
    CPUTFoliagePatchMeshDX11 *mpMesh;
    FoliagePatchSortNode     *mpRight;
    FoliagePatchSortNode     *mpLeft;

    FoliagePatchSortNode() :
        mIndex(-1),
        mpMesh(NULL),
        mDistance(0.0f),
        mpRight(NULL),
        mpLeft(NULL)
    {
    }
    FoliagePatchSortNode(CPUTFoliagePatchMeshDX11 *pMesh, float distance) :
        mpMesh(pMesh),
        mDistance(distance),
        mpRight(NULL),
        mpLeft(NULL)
    {
    }
    void Init( float distance )
    {
        // Leave index.  We set it at create time, and never change it.
        // mIndex = index;
        mpRight = mpLeft = NULL;
        mDistance = distance;
    }
    void Insert( FoliagePatchSortNode *pNode )
    {
        if( pNode->mDistance > mDistance )
        {
            if( mpRight )
            {
                mpRight->Insert( pNode );
            } else
            {
                mpRight = pNode;
            }
        } else
        {
            if( mpLeft )
            {
                mpLeft->Insert( pNode );
            } else
            {
                mpLeft = pNode;
            }
        }
    }
    ~FoliagePatchSortNode() {}
    void Render( CPUTRenderParameters &renderParams, CPUTFoliagePatchDX11 *pPatch );
};

//--------------------------------------------------------------------------------------
class CPUTFoliagePatchDX11 : public CPUTModelDX11
{
    friend class CPUTFoliagePatchMeshDX11;
    friend class FoliagePatchSortNode;

protected:
    CPUTAssetSetDX11          **mp3DPlantAssetSet;
    CPUTTexture                *mpHeightTexture;
    CPUTTexture               **mpProbabilityTexture;
    bool                       *mpProbabilityTextureMapped;
    CPUTTexture                *mpFoliageBillboardTextureArrayColor;
    CPUTTexture                *mpFoliageBillboardTextureArrayNormal;
    CPUTTexture                *mpFoliageBillboardTextureArrayProbability;
    CPUTFoliagePlantParameters *mpFoliagePlantParameters;
    cString                    *mpFilenameColorList;
    cString                   **mpFilenameColorPtrList;
    cString                    *mpFilenameNormalList;
    cString                   **mpFilenameNormalPtrList;
    cString                    *mpFilenameProbabilityList;
    cString                   **mpFilenameProbabilityPtrList;
    cString                     mHeightTextureName;
    cString                     mMaterialName;
    int                         mNumPlantTypes;
    int                         mWidthInPlants;
    int                         mHeightInPlants;
    int                         mWidthInPatches;
    int                         mHeightInPatches;
    float                       mHeightScale;
    float                       mHeightOffset;
    float                       mHeightTextureWorldWidth;
    float                       mHeightTextureWorldHeight;
    float                       mWorldWidth;
    float                       mWorldHeight;
    float                       mUScale;
    float                       mVScale;
    USHORT                     *mpHeightTexels;
    UCHAR                     **mpProbabilityTexels; // One for each plant type
    int                         mHeightTextureWidth;
    int                         mHeightTextureHeight;
    int                         mProbabilityTextureWidth;
    int                         mProbabilityTextureHeight;
    int                       **mpPlantLast3DFrameTime; // Last frame index when each plant was rendered as 3D
    float4x4                  **mpPlantLast3DParentMatrix;
    FoliagePatchSortNode       *mpPatchSortRootNode;
    FoliagePatchSortNode       *mpPatchSortNodes;

    CPUTResult LoadFoliagePatchParameters( const cString &name, CPUTRenderParametersDX &params );
    void       GetPlantPositionAndBbox( 
                    int     plantType,
                    USHORT *pHeightTexels,
                    UCHAR **pProbabilityTexels,
                    UINT    heightTextureWidth,
                    UINT    heightTextureHeight,
                    UINT    probabilityTextureWidth,
                    UINT    probabilityTextureHeight,
                    int     gridIndexX,
                    int     gridIndexY,
                    float3 *pPosition,
                    float3 *pBboxCenter,
                    float3 *pBboxHalf,
                    bool   *pPlantActive
                );
    void UpdateShaderConstants( CPUTRenderParameters &renderParams, const float3 &patchCenterPosition );
    int  SortIndex( int *pX, int *pY, int index, CPUTCamera *pCamera );
    void RenderBillboardPatches( CPUTRenderParameters &renderParams, int materialIndex);

public:
    CPUTFoliagePatchDX11() :
        mp3DPlantAssetSet(NULL),
        mpHeightTexture(NULL),
        mpProbabilityTexture(NULL),
        mpProbabilityTextureMapped(NULL),
        mpFoliageBillboardTextureArrayColor(NULL),
        mpFoliageBillboardTextureArrayNormal(NULL),
        mpFoliageBillboardTextureArrayProbability(NULL),
        mpFoliagePlantParameters(NULL),
        mpFilenameColorList(NULL),
        mpFilenameColorPtrList(NULL),
        mpFilenameNormalList(NULL),
        mpFilenameNormalPtrList(NULL),
        mpFilenameProbabilityList(NULL),
        mpFilenameProbabilityPtrList(NULL),
        mNumPlantTypes(-1),
        mHeightInPlants(128),
        mWidthInPlants(128),
        mWidthInPatches(4),
        mHeightInPatches(4),
        mHeightScale(600.0),
        mHeightOffset(0.0f),
        mHeightTextureWorldWidth(5285.28f), // TODO: These values are hard-coded for our scene.  Move them to data.
        mHeightTextureWorldHeight(5285.28f),
        mWorldWidth(5273.76f),
        mWorldHeight(5273.76f),
        mUScale(1.0f/5273.76f),
        mVScale(1.0f/5273.76f),
        mpHeightTexels(NULL),
        mpProbabilityTexels(NULL),
        mHeightTextureWidth(0),
        mHeightTextureHeight(0),
        mProbabilityTextureWidth(0),
        mProbabilityTextureHeight(0),
        mpPlantLast3DFrameTime(NULL),
        mpPlantLast3DParentMatrix(NULL),
        mpPatchSortRootNode(NULL),
        mpPatchSortNodes(NULL)
    {}
    ~CPUTFoliagePatchDX11() 
	{
		SAFE_RELEASE(mpHeightTexture);
		SAFE_RELEASE(mpFoliageBillboardTextureArrayColor);
		SAFE_RELEASE(mpFoliageBillboardTextureArrayNormal);
		SAFE_RELEASE(mpFoliageBillboardTextureArrayProbability);
		SAFE_DELETE_ARRAY(mpFoliagePlantParameters);
		SAFE_DELETE_ARRAY(mpProbabilityTexels);
		SAFE_DELETE_ARRAY(mpFilenameProbabilityList);
		SAFE_DELETE_ARRAY(mpFilenameProbabilityPtrList);
		SAFE_DELETE_ARRAY(mpFilenameNormalPtrList);
		SAFE_DELETE_ARRAY(mpFilenameNormalList);
		SAFE_DELETE_ARRAY(mpFilenameColorPtrList);
		SAFE_DELETE_ARRAY(mpFilenameColorList);

        CPUTRenderParametersDX params;
        params.mpContext = gpSample->GetContext();
		for(int ii=0;ii<mNumPlantTypes;ii++)
		{
            if( mpProbabilityTexture[ii] && mpProbabilityTextureMapped[ii] )
            {
                mpProbabilityTexture[ii]->UnmapTexture( params );
            }
			SAFE_RELEASE(mpProbabilityTexture[ii]);
			SAFE_RELEASE(mp3DPlantAssetSet[ii]);
		}
	
		SAFE_DELETE_ARRAY(mpProbabilityTexture);
        SAFE_DELETE_ARRAY(mpProbabilityTextureMapped);
		SAFE_DELETE_ARRAY(mp3DPlantAssetSet);

		for( int ii=0; ii<mNumPlantTypes; ii++ )
		{
			SAFE_DELETE_ARRAY(mpPlantLast3DFrameTime[ii]);
			SAFE_DELETE_ARRAY(mpPlantLast3DParentMatrix[ii]);
		}
		SAFE_DELETE_ARRAY(mpPlantLast3DFrameTime);
		SAFE_DELETE_ARRAY(mpPlantLast3DParentMatrix);
        SAFE_DELETE_ARRAY(mpPatchSortNodes);
    }
    CPUTResult LoadFoliagePatch( const cString &name, CPUTRenderParametersDX &params );
    void Render( CPUTRenderParameters &renderParams, int materialIndex=0 );
    void RenderShadows( CPUTRenderParameters &renderParams, int materialIndex );
};
#endif // __CPUTFOLIAGEPATCHDX11_H__
