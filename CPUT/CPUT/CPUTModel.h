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
#ifndef __CPUTMODEL_H__
#define __CPUTMODEL_H__

// Define the following to support drawing bounding boxes.
// Note that you also need bounding-box materials and shaders.  TODO: Include them in the exe.
#define SUPPORT_DRAWING_BOUNDING_BOXES 1

#include "CPUTRenderNode.h"
#include "CPUTMath.h"
#include "CPUTConfigBlock.h"
#include "CPUTMesh.h"

class CPUTMaterial;
class CPUTMesh;

enum {
    CPUT_MATERIAL_INDEX_SHADOW_CAST                 = -1,
    CPUT_MATERIAL_INDEX_AMBIENT_SHADOW_CAST         = -2,
    CPUT_MATERIAL_INDEX_HEIGHT_FIELD                = -3,
    CPUT_MATERIAL_INDEX_BOUNDING_BOX                = -4,
    CPUT_MATERIAL_INDEX_TERRAIN_LIGHT_MAP           = -5,
    CPUT_MATERIAL_INDEX_TERRAIN_AND_TREES_LIGHT_MAP = -6
};
const int NUM_GLOBAL_SYSTEM_MATERIALS = 6;

//-----------------------------------------------------------------------------
class CPUTModel : public CPUTRenderNode
{
protected:  
    CPUTMesh      **mpMesh;
    UINT           *mpSubMaterialCount;
    CPUTMaterial ***mpMaterial; // Have arrays of submaterials, one array for each mesh.

    UINT            mMeshCount;
    bool            mIsRenderable;
    float3          mBoundingBoxCenterObjectSpace;
    float3          mBoundingBoxHalfObjectSpace;
    float3          mBoundingBoxCenterWorldSpace;
    float3          mBoundingBoxHalfWorldSpace;
    CPUTMesh       *mpBoundingBoxMesh;

public:
    CPUTModel():
        mMeshCount(0),
        mpSubMaterialCount(NULL),
        mpMaterial(NULL),
        mpMesh(NULL),
        mIsRenderable(true),
        mBoundingBoxCenterObjectSpace(0.0f),
        mBoundingBoxHalfObjectSpace(0.0f),
        mBoundingBoxCenterWorldSpace(0.0f),
        mBoundingBoxHalfWorldSpace(0.0f),
        mpBoundingBoxMesh(NULL)
    {}
    virtual ~CPUTModel();

    bool               IsRenderable() { return mIsRenderable; }
    void               SetRenderable(bool isRenderable) { mIsRenderable = isRenderable; }
    virtual bool       IsModel() { return true; }
    void               GetBoundsObjectSpace(float3 *pCenter, float3 *pHalf);
    void               GetBoundsWorldSpace(float3 *pCenter, float3 *pHalf);
    void               UpdateBoundsWorldSpace();
    int                GetMeshCount() const { return mMeshCount; }
    CPUTMesh          *GetMesh( UINT ii ) { return mpMesh[ii]; }
    virtual CPUTResult LoadModel(CPUTConfigBlock *pBlock, int *pParentID, CPUTModel *pMasterModel=NULL, int numSystemMaterials=0, cString *pSystemMaterialNames=NULL) = 0;
    CPUTResult         LoadModelPayload(const cString &File);
    virtual void       SetSubMaterials(UINT ii, CPUTMaterial **pMaterial);
    UINT GetMaterialIndex(int meshIndex, int index)
    {
        return index >= 0 ? index : mpSubMaterialCount[meshIndex] + index;
    }

#ifdef SUPPORT_DRAWING_BOUNDING_BOXES
    virtual void       DrawBoundingBox(CPUTRenderParameters &renderParams) = 0;
    virtual void       CreateBoundingBoxMesh() = 0;
#endif

    void GetBoundingBoxRecursive( float3 *pCenter, float3 *pHalf)
    {
        if( *pHalf == float3(0.0f) )
        {
            *pCenter = mBoundingBoxCenterWorldSpace;
            *pHalf   = mBoundingBoxHalfWorldSpace;
        }
        else
        {
            float3 minExtent = *pCenter - *pHalf;
            float3 maxExtent = *pCenter + *pHalf;
            minExtent = Min( (mBoundingBoxCenterWorldSpace - mBoundingBoxHalfWorldSpace), minExtent );
            maxExtent = Max( (mBoundingBoxCenterWorldSpace + mBoundingBoxHalfWorldSpace), maxExtent );
            *pCenter = (minExtent + maxExtent) * 0.5f;
            *pHalf   = (maxExtent - minExtent) * 0.5f;
        }
        if(mpChild)   { mpChild->GetBoundingBoxRecursive(   pCenter, pHalf ); }
        if(mpSibling) { mpSibling->GetBoundingBoxRecursive( pCenter, pHalf ); }
    }
};
#endif // __CPUTMODEL_H__