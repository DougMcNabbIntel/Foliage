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
#ifndef __CPUTMATERIAL_H__
#define __CPUTMATERIAL_H__

#include <stdio.h>
#include "CPUT.h"
#include "CPUTRefCount.h"
#include "CPUTConfigBlock.h"
#include "CPUTTexture.h"
#include "CPUTRenderStateBlock.h"

class CPUTShaderParameters;

// TODO: Where did this number come frome?  It should also be different for each API
#define CPUT_MATERIAL_MAX_TEXTURE_SLOTS         32
#define CPUT_MATERIAL_MAX_BUFFER_SLOTS          32
#define CPUT_MATERIAL_MAX_CONSTANT_BUFFER_SLOTS 32
#define CPUT_MATERIAL_MAX_SRV_SLOTS             32

#if 1 // Need to handle >=DX11 vs. < DX11, where max UAV slots == 1;
#   define CPUT_MATERIAL_MAX_UAV_SLOTS             8
#else
#   define CPUT_MATERIAL_MAX_UAV_SLOTS             1
#endif

class CPUTMaterial:public CPUTRefCount
{
protected:
    cString               mMaterialName;
    CPUTConfigBlock       mConfigBlock;
    CPUTRenderStateBlock *mpRenderStateBlock;
    // A material can have multiple submaterials.  If it does, then that's all it does.  It just "branches" to other materials.
    // TODO: We could make that a special object, and derive them from material.  Not sure that's worth the effort.
    // The alternative we choose here is to simply comandeer this one, ignoring most of its state and functionality.
    int                   mSubMaterialCount;
    CPUTMaterial        **mpSubMaterials; 
    cString              *mpSubMaterialNames;

    // Destructor is not public.  Must release instead of delete.
    virtual ~CPUTMaterial()
	{
        if( mpSubMaterials )
        {
            for( int ii=0; ii<mSubMaterialCount; ii++ )
            {
                SAFE_RELEASE(mpSubMaterials[ii]);
            }
            SAFE_DELETE_ARRAY(mpSubMaterials);
            SAFE_DELETE_ARRAY(mpSubMaterialNames);
        }
	}

public:
    static CPUTMaterial *CreateMaterial(
        const cString &absolutePathAndFilename,
        const cString &modelSuffix,
        const cString &meshSuffix,
              char  **pShaderMacros=NULL,
              int     numSystemMaterials=0,
             cString *pSystemMaterialNames=NULL
 );
    static CPUTConfigBlock mGlobalProperties;

    CPUTMaterial() :
        mpRenderStateBlock(NULL),
        mSubMaterialCount(0),
        mpSubMaterials(NULL),
        mpSubMaterialNames(NULL)
    {
    }

    void                  SetMaterialName(const cString MaterialName) { mMaterialName = MaterialName; }
    void                  GetMaterialName(cString &MaterialName)      { MaterialName   = mMaterialName; }
    virtual void          ReleaseTexturesAndBuffers() = 0;
    virtual void          RebindTexturesAndBuffers() = 0;
    virtual void          SetRenderStates(CPUTRenderParameters &renderParams) { if( mpRenderStateBlock ) { mpRenderStateBlock->SetRenderStates(renderParams); } }
    virtual bool          MaterialRequiresPerModelPayload() = 0;
    virtual CPUTMaterial *CloneMaterial( const cString &absolutePathAndFilename, const cString &modelSuffix, const cString &meshSuffix ) = 0;
    CPUTMaterial **GetSubMaterials() { return mpSubMaterials; }
    bool IsMultiMaterial() const { return mSubMaterialCount > 1; }
    UINT GetSubMaterialCount() { return mSubMaterialCount; }

    virtual CPUTResult LoadMaterial(
        const cString   &fileName,
        const cString   &modelSuffix,
        const cString   &meshSuffix,
              char    **pShaderMacros=NULL,
              int       systemMaterialCount=0,
              cString  *pSystemMaterialNames=NULL
    ) = 0;

};

#endif //#ifndef __CPUTMATERIAL_H__
