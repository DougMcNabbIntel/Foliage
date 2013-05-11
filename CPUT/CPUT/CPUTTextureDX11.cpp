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

#include "CPUTTextureDX11.h"

// TODO: Would be nice to find a better place for this decl.  But, not another file just for this.
const cString gDXGIFormatNames[] =
{
    _L("DXGI_FORMAT_UNKNOWN"),
    _L("DXGI_FORMAT_R32G32B32A32_TYPELESS"),
    _L("DXGI_FORMAT_R32G32B32A32_FLOAT"),
    _L("DXGI_FORMAT_R32G32B32A32_UINT"),
    _L("DXGI_FORMAT_R32G32B32A32_SINT"),
    _L("DXGI_FORMAT_R32G32B32_TYPELESS"),
    _L("DXGI_FORMAT_R32G32B32_FLOAT"),
    _L("DXGI_FORMAT_R32G32B32_UINT"),
    _L("DXGI_FORMAT_R32G32B32_SINT"),
    _L("DXGI_FORMAT_R16G16B16A16_TYPELESS"),
    _L("DXGI_FORMAT_R16G16B16A16_FLOAT"),
    _L("DXGI_FORMAT_R16G16B16A16_UNORM"),
    _L("DXGI_FORMAT_R16G16B16A16_UINT"),
    _L("DXGI_FORMAT_R16G16B16A16_SNORM"),
    _L("DXGI_FORMAT_R16G16B16A16_SINT"),
    _L("DXGI_FORMAT_R32G32_TYPELESS"),
    _L("DXGI_FORMAT_R32G32_FLOAT"),
    _L("DXGI_FORMAT_R32G32_UINT"),
    _L("DXGI_FORMAT_R32G32_SINT"),
    _L("DXGI_FORMAT_R32G8X24_TYPELESS"),
    _L("DXGI_FORMAT_D32_FLOAT_S8X24_UINT"),
    _L("DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS"),
    _L("DXGI_FORMAT_X32_TYPELESS_G8X24_UINT"),
    _L("DXGI_FORMAT_R10G10B10A2_TYPELESS"),
    _L("DXGI_FORMAT_R10G10B10A2_UNORM"),
    _L("DXGI_FORMAT_R10G10B10A2_UINT"),
    _L("DXGI_FORMAT_R11G11B10_FLOAT"),
    _L("DXGI_FORMAT_R8G8B8A8_TYPELESS"),
    _L("DXGI_FORMAT_R8G8B8A8_UNORM"),
    _L("DXGI_FORMAT_R8G8B8A8_UNORM_SRGB"),
    _L("DXGI_FORMAT_R8G8B8A8_UINT"),
    _L("DXGI_FORMAT_R8G8B8A8_SNORM"),
    _L("DXGI_FORMAT_R8G8B8A8_SINT"),
    _L("DXGI_FORMAT_R16G16_TYPELESS"),
    _L("DXGI_FORMAT_R16G16_FLOAT"),
    _L("DXGI_FORMAT_R16G16_UNORM"),
    _L("DXGI_FORMAT_R16G16_UINT"),
    _L("DXGI_FORMAT_R16G16_SNORM"),
    _L("DXGI_FORMAT_R16G16_SINT"),
    _L("DXGI_FORMAT_R32_TYPELESS"),
    _L("DXGI_FORMAT_D32_FLOAT"),
    _L("DXGI_FORMAT_R32_FLOAT"),
    _L("DXGI_FORMAT_R32_UINT"),
    _L("DXGI_FORMAT_R32_SINT"),
    _L("DXGI_FORMAT_R24G8_TYPELESS"),
    _L("DXGI_FORMAT_D24_UNORM_S8_UINT"),
    _L("DXGI_FORMAT_R24_UNORM_X8_TYPELESS"),
    _L("DXGI_FORMAT_X24_TYPELESS_G8_UINT"),
    _L("DXGI_FORMAT_R8G8_TYPELESS"),
    _L("DXGI_FORMAT_R8G8_UNORM"),
    _L("DXGI_FORMAT_R8G8_UINT"),
    _L("DXGI_FORMAT_R8G8_SNORM"),
    _L("DXGI_FORMAT_R8G8_SINT"),
    _L("DXGI_FORMAT_R16_TYPELESS"),
    _L("DXGI_FORMAT_R16_FLOAT"),
    _L("DXGI_FORMAT_D16_UNORM"),
    _L("DXGI_FORMAT_R16_UNORM"),
    _L("DXGI_FORMAT_R16_UINT"),
    _L("DXGI_FORMAT_R16_SNORM"),
    _L("DXGI_FORMAT_R16_SINT"),
    _L("DXGI_FORMAT_R8_TYPELESS"),
    _L("DXGI_FORMAT_R8_UNORM"),
    _L("DXGI_FORMAT_R8_UINT"),
    _L("DXGI_FORMAT_R8_SNORM"),
    _L("DXGI_FORMAT_R8_SINT"),
    _L("DXGI_FORMAT_A8_UNORM"),
    _L("DXGI_FORMAT_R1_UNORM"),
    _L("DXGI_FORMAT_R9G9B9E5_SHAREDEXP"),
    _L("DXGI_FORMAT_R8G8_B8G8_UNORM"),
    _L("DXGI_FORMAT_G8R8_G8B8_UNORM"),
    _L("DXGI_FORMAT_BC1_TYPELESS"),
    _L("DXGI_FORMAT_BC1_UNORM"),
    _L("DXGI_FORMAT_BC1_UNORM_SRGB"),
    _L("DXGI_FORMAT_BC2_TYPELESS"),
    _L("DXGI_FORMAT_BC2_UNORM"),
    _L("DXGI_FORMAT_BC2_UNORM_SRGB"),
    _L("DXGI_FORMAT_BC3_TYPELESS"),
    _L("DXGI_FORMAT_BC3_UNORM"),
    _L("DXGI_FORMAT_BC3_UNORM_SRGB"),
    _L("DXGI_FORMAT_BC4_TYPELESS"),
    _L("DXGI_FORMAT_BC4_UNORM"),
    _L("DXGI_FORMAT_BC4_SNORM"),
    _L("DXGI_FORMAT_BC5_TYPELESS"),
    _L("DXGI_FORMAT_BC5_UNORM"),
    _L("DXGI_FORMAT_BC5_SNORM"),
    _L("DXGI_FORMAT_B5G6R5_UNORM"),
    _L("DXGI_FORMAT_B5G5R5A1_UNORM"),
    _L("DXGI_FORMAT_B8G8R8A8_UNORM"),
    _L("DXGI_FORMAT_B8G8R8X8_UNORM"),
    _L("DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM"),
    _L("DXGI_FORMAT_B8G8R8A8_TYPELESS"),
    _L("DXGI_FORMAT_B8G8R8A8_UNORM_SRGB"),
    _L("DXGI_FORMAT_B8G8R8X8_TYPELESS"),
    _L("DXGI_FORMAT_B8G8R8X8_UNORM_SRGB"),
    _L("DXGI_FORMAT_BC6H_TYPELESS"),
    _L("DXGI_FORMAT_BC6H_UF16"),
    _L("DXGI_FORMAT_BC6H_SF16"),
    _L("DXGI_FORMAT_BC7_TYPELESS"),
    _L("DXGI_FORMAT_BC7_UNORM"),
    _L("DXGI_FORMAT_BC7_UNORM_SRGB")
};
const cString *gpDXGIFormatNames = gDXGIFormatNames;

//-----------------------------------------------------------------------------
CPUTTexture *CPUTTextureDX11::CreateTexture( const cString &name, const cString &absolutePathAndFilename, bool loadAsSRGB )
{
    // TODO:  Delegate to derived class.  We don't currently have CPUTTextureDX11
    ID3D11ShaderResourceView *pShaderResourceView = NULL;
    ID3D11Resource *pTexture   = NULL;
    ID3D11Device   *pD3dDevice = CPUT_DX11::GetDevice();
    CPUTResult result = CreateNativeTextureFromFile( pD3dDevice, absolutePathAndFilename, &pShaderResourceView, &pTexture, loadAsSRGB );
    ASSERT( CPUTSUCCESS(result), _L("Error loading texture: '")+absolutePathAndFilename );
    UNREFERENCED_PARAMETER(result);

    CPUTTextureDX11 *pNewTexture = new CPUTTextureDX11();
    pNewTexture->mName = name;
    pNewTexture->SetTextureAndShaderResourceView( pTexture, pShaderResourceView );
    pTexture->Release();
    pShaderResourceView->Release();

    CPUTAssetLibrary::GetAssetLibrary()->AddTexture( absolutePathAndFilename, pNewTexture);

    return pNewTexture;
}

//-----------------------------------------------------------------------------
CPUTTexture *CPUTTextureDX11::CreateTextureArrayFromTxaFile( CPUTRenderParametersDX &renderParams, cString &absolutePathAndFilename, bool loadAsSRGB )
{
    CPUTAssetLibraryDX11 *pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();

    // Loop over the list to count 
    cString textureDirectoryName = pAssetLibrary->GetTextureDirectoryName();
    // Convert to absolute path and filename
    cString name0 = textureDirectoryName + _L("Grass.dds");
    cString name1 = textureDirectoryName + _L("Grass.dds");
    cString name2 = textureDirectoryName + _L("Grass.dds");
    cString name3 = textureDirectoryName + _L("Grass.dds");
    cString name4 = textureDirectoryName + _L("Grass.dds");
    cString name5 = textureDirectoryName + _L("Grass.dds");
    cString name6 = textureDirectoryName + _L("Grass.dds");
    cString name7 = textureDirectoryName + _L("Grass.dds");
    cString *pFilenameList[] = {
        &name0,
        &name1,
        &name2,
        &name3,
        &name4,
        &name5,
        &name6,
        &name7,
        NULL
    };
    CPUTTexture *pNewTexture = CreateTextureArrayFromFilenameList( renderParams, absolutePathAndFilename, pFilenameList, loadAsSRGB );
    if( pNewTexture )
    {
        pAssetLibrary->AddTexture( absolutePathAndFilename, pNewTexture );
    }
    return pNewTexture;
}

//-----------------------------------------------------------------------------
CPUTTexture *CPUTTextureDX11::CreateTextureArrayFromFilenameList(
    CPUTRenderParametersDX &renderParams,
    cString &absolutePathAndFilename, // Name of this texture array
    cString **pFilenameList, // List of names; one for each texture of the array
    bool loadAsSRGB
){
    CPUTAssetLibraryDX11 *pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();

    // TODO:  Delegate to derived class.  We don't currently have CPUTTextureDX11
    ID3D11ShaderResourceView *pShaderResourceView = NULL;
    ID3D11Resource *pTexture = NULL;
    ID3D11Device *pD3dDevice = CPUT_DX11::GetDevice();

    int  indexFirstRealTexture = -1;
    UINT numArraySlices;
    for( numArraySlices=0; pFilenameList[numArraySlices]; numArraySlices++ )
    {
        // This loop looks for the first NULL entry in the filename list.
        // When complete, numArraySlices contains the index of that first null entry
        if( indexFirstRealTexture == -1 && *pFilenameList[numArraySlices] != _L("$Undefined") )
        {
            indexFirstRealTexture = numArraySlices;
        }
    }
    if( indexFirstRealTexture == -1 )
    {
        // No real textures specified (i.e., all == Undefined)
        return NULL;
    }

    // Get information we need from first slice texture.  We need information like the texture format.
    // Note that all slices must have the same dimensions, type, format, etc.
    D3DX11_IMAGE_INFO srcInfo;
    HRESULT hr = D3DX11GetImageInfoFromFile( pFilenameList[indexFirstRealTexture]->c_str(), NULL, &srcInfo, NULL);
    ASSERT( SUCCEEDED(hr), _L(" - Error loading texture '") + *pFilenameList[indexFirstRealTexture] + _L("'.") );
    UNREFERENCED_PARAMETER(hr);

    CPUTTextureDX11 *pNewTexture = new CPUTTextureDX11( absolutePathAndFilename, numArraySlices ); // TODO: Move numArraySlices to Create* call
    CPUTResult result = pNewTexture->CreateNativeTexture( pD3dDevice, absolutePathAndFilename, srcInfo, &pShaderResourceView, &pTexture, loadAsSRGB );
    ASSERT( CPUTSUCCESS(result), _L("Error creating texture array: '") + absolutePathAndFilename );
    UNREFERENCED_PARAMETER(result);

    pNewTexture->SetTextureAndShaderResourceView( pTexture, pShaderResourceView );
    pTexture->Release();
    pShaderResourceView->Release();

    UINT numMipLevels = srcInfo.MipLevels;
    for( UINT ii=0; ii<numArraySlices; ii++ )
    {
        if( *pFilenameList[ii] == _L("$Undefined") )
        {
            // No texture for this slice.
            // TODO: use indirection table to avoid this, and to avoid duplicating slices
            continue;
        }

        for( UINT mip=0; mip<srcInfo.MipLevels; mip++ )
        {
            // Get the texture by name
            CPUTTextureDX11 *pTexture = (CPUTTextureDX11*)pAssetLibrary->GetTexture( *pFilenameList[ii], true, loadAsSRGB );
            ASSERT( pTexture, _L("Error getting texture ") + *pFilenameList[ii] );
#ifdef _DEBUG
            // TODO: Add GetNumMipLevels() to CPUTTexture
            D3DX11GetImageInfoFromFile( pFilenameList[ii]->c_str(), NULL, &srcInfo, NULL);
            ASSERT( numMipLevels == srcInfo.MipLevels, _L("Array textures must have the same MIP count") );
#endif
            // Set this texture to our array texture at slice ii
            UINT srcSubresource  = D3D11CalcSubresource( mip, 0,  numMipLevels );
            UINT destSubresource = D3D11CalcSubresource( mip, ii, numMipLevels );
            renderParams.mpContext->CopySubresourceRegion(
                pNewTexture->mpTexture,
                destSubresource,
                0, 0, 0,
                pTexture->mpTexture,
                srcSubresource,
                NULL
            );
			pTexture->Release();
        }
    }
    return pNewTexture;
}

//-----------------------------------------------------------------------------
CPUTResult CPUTTextureDX11::CreateNativeTexture(
    ID3D11Device *pD3dDevice,
    const cString &fileName,
    D3DX11_IMAGE_INFO &srcInfo,
    ID3D11ShaderResourceView **ppShaderResourceView,
    ID3D11Resource **ppTexture,
    bool forceLoadAsSRGB
){
    CPUTResult result = CPUT_SUCCESS;
    HRESULT hr;

    DXGI_FORMAT format = srcInfo.Format;
    if( forceLoadAsSRGB )
    {
        GetSRGBEquivalent(srcInfo.Format, format);
    }
    DXGI_SAMPLE_DESC sampleDesc = {1,0};
    D3D11_TEXTURE2D_DESC desc;
    desc.Width          = srcInfo.Width;
    desc.Height         = srcInfo.Height;
    desc.MipLevels      = srcInfo.MipLevels;
    desc.ArraySize      = mNumArraySlices;
    desc.Format         = format;
    desc.SampleDesc     = sampleDesc;
    desc.Usage          = D3D11_USAGE_DEFAULT;
    desc.BindFlags      = D3D11_BIND_SHADER_RESOURCE; // TODO: Support UAV, etc.  Accept flags as argument?
    desc.CPUAccessFlags = 0;
    desc.MiscFlags      = srcInfo.MiscFlags;

    hr = pD3dDevice->CreateTexture2D( &desc, NULL, (ID3D11Texture2D**)ppTexture );
    ASSERT( SUCCEEDED(hr), _L("Failed to create texture array: ") + fileName );
    CPUTSetDebugName( *ppTexture, fileName );

    hr = pD3dDevice->CreateShaderResourceView( *ppTexture, NULL, ppShaderResourceView );
    ASSERT( SUCCEEDED(hr), _L("Failed to create texture shader resource view.") );
    CPUTSetDebugName( *ppShaderResourceView, fileName );

    return result;
}

//-----------------------------------------------------------------------------
CPUTResult CPUTTextureDX11::CreateNativeTextureFromFile(
    ID3D11Device *pD3dDevice,
    const cString &fileName,
    ID3D11ShaderResourceView **ppShaderResourceView,
    ID3D11Resource **ppTexture,
    bool forceLoadAsSRGB
){
    CPUTResult result;
    HRESULT hr;

    // Set up loading structure
    //
    // Indicate all texture parameters should come from the file
    D3DX11_IMAGE_LOAD_INFO loadInfo;
    ZeroMemory(&loadInfo, sizeof(D3DX11_IMAGE_LOAD_INFO));
    loadInfo.Width          = D3DX11_FROM_FILE;
    loadInfo.Height         = D3DX11_FROM_FILE;
    loadInfo.Depth          = D3DX11_FROM_FILE;
    loadInfo.FirstMipLevel  = D3DX11_FROM_FILE;
    loadInfo.MipLevels      = D3DX11_FROM_FILE;
    // loadInfo.Usage          = D3D11_USAGE_IMMUTABLE; // TODO: maintain a "mappable" flag?  Set immutable if not mappable?
    loadInfo.Usage          = D3D11_USAGE_DEFAULT;
    loadInfo.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    loadInfo.CpuAccessFlags = 0;
    loadInfo.MiscFlags      = 0;
    loadInfo.MipFilter      = D3DX11_FROM_FILE;
    loadInfo.pSrcInfo       = NULL;
    loadInfo.Format         = (DXGI_FORMAT) D3DX11_FROM_FILE;
    loadInfo.Filter         = D3DX11_FILTER_NONE;

    // if we're 'forcing' load of sRGB data, we need to verify image is sRGB
    // or determine image format that best matches the non-sRGB source format in hopes that the conversion will be faster
    // and data preserved
    if(true == forceLoadAsSRGB)
    {
        // get the source image info
        D3DX11_IMAGE_INFO srcInfo;
        hr = D3DX11GetImageInfoFromFile(fileName.c_str(), NULL, &srcInfo, NULL);
        ASSERT( SUCCEEDED(hr), _L(" - Error loading texture '") + fileName + _L("'.") );

        // find a closest equivalent sRGB format
        result = GetSRGBEquivalent(srcInfo.Format, loadInfo.Format);
        ASSERT( CPUTSUCCESS(result), _L("Error loading texture '") + fileName + _L("'.  It is specified this texture must load as sRGB, but the source image is in a format that cannot be converted to sRGB.\n") );

        // set filtering mode to interpret 'in'-coming data as sRGB, and storing it 'out' on an sRGB surface
        //
        // As it stands, we don't have any tools that support sRGB output in DXT compressed textures.
        // If we later support a format that does provide sRGB, then the filter 'in' flag will need to be removed
        loadInfo.Filter = D3DX11_FILTER_NONE | D3DX11_FILTER_SRGB_IN | D3DX11_FILTER_SRGB_OUT;
#if 0
        // DWM: TODO:  We want to catch the cases where the loader needs to do work.
        // This happens if the texture's pixel format isn't supported by DXGI.
        // TODO: how to determine?

        // if a runtime conversion must happen report a performance warning error.
        // Note: choosing not to assert here, as this will be a common issue.
        if( srcInfo.Format != loadInfo.Format)
        {
            cString dxgiName = GetDXGIFormatString(srcInfo.Format);
            cString errorString = _T(__FUNCTION__);
            errorString += _L("- PERFORMANCE WARNING: '") + fileName
            +_L("' has an image format ")+dxgiName
            +_L(" but must be run-time converted to ")+GetDXGIFormatString(loadInfo.Format)
            +_L(" based on requested sRGB target buffer.\n");
            TRACE( errorString.c_str() );
        }
#endif
    }
    hr = D3DX11CreateTextureFromFile( pD3dDevice, fileName.c_str(), &loadInfo, NULL, ppTexture, NULL );
    ASSERT( SUCCEEDED(hr), _L("Failed to load texture: ") + fileName );
    CPUTSetDebugName( *ppTexture, fileName );

    hr = pD3dDevice->CreateShaderResourceView( *ppTexture, NULL, ppShaderResourceView );
    ASSERT( SUCCEEDED(hr), _L("Failed to create texture shader resource view.") );
    CPUTSetDebugName( *ppShaderResourceView, fileName );

    return CPUT_SUCCESS;
}

//-----------------------------------------------------------------------------
CPUTResult CPUTTextureDX11::GetSRGBEquivalent(DXGI_FORMAT inFormat, DXGI_FORMAT& sRGBFormat)
{
    switch( inFormat )
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            sRGBFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            return CPUT_SUCCESS;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            sRGBFormat = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
            return CPUT_SUCCESS;
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            sRGBFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
            return CPUT_SUCCESS;
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            sRGBFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
            return CPUT_SUCCESS;
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            sRGBFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
            return CPUT_SUCCESS;
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            sRGBFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
            return CPUT_SUCCESS;
    };
    return CPUT_ERROR_UNSUPPORTED_SRGB_IMAGE_FORMAT;
}

// This function returns the DXGI string equivalent of the DXGI format for
// error reporting/display purposes
//-----------------------------------------------------------------------------
const cString &CPUTTextureDX11::GetDXGIFormatString(DXGI_FORMAT format)
{
    ASSERT( (format>=0) && (format<=DXGI_FORMAT_BC7_UNORM_SRGB), _L("Invalid DXGI Format.") );
    return gpDXGIFormatNames[format];
}

// Given a certain DXGI texture format, does it even have an equivalent sRGB one
//-----------------------------------------------------------------------------
bool CPUTTextureDX11::DoesExistEquivalentSRGBFormat(DXGI_FORMAT inFormat)
{
    DXGI_FORMAT outFormat;

    if( CPUT_ERROR_UNSUPPORTED_SRGB_IMAGE_FORMAT == GetSRGBEquivalent(inFormat, outFormat) )
    {
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
D3D11_MAPPED_SUBRESOURCE CPUTTextureDX11::MapTexture( CPUTRenderParameters &params, eCPUTMapType type, bool wait )
{
    // Mapping for DISCARD requires dynamic buffer.  Create dynamic copy?
    // Could easily provide input flag.  But, where would we specify? Don't like specifying in the .set file
    // Because mapping is something the application wants to do - it isn't inherent in the data.
    // Could do Clone() and pass dynamic flag to that.
    // But, then we have two.  Could always delete the other.
    // Could support programatic flag - apply to all loaded models in the .set
    // Could support programatic flag on model.  Load model first, then load set.
    // For now, simply support CopyResource mechanism.
    HRESULT hr;
    ID3D11Device *pD3dDevice = CPUT_DX11::GetDevice();
    CPUTRenderParametersDX *pParamsDX11 = (CPUTRenderParametersDX*)&params;
    ID3D11DeviceContext *pContext = pParamsDX11->mpContext;

    if( !mpTextureStaging )
    {
        // Annoying.  We need to create the texture differently, based on dimension.
        D3D11_RESOURCE_DIMENSION dimension;
        mpTexture->GetType(&dimension);
        switch( dimension )
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            {
                D3D11_TEXTURE1D_DESC desc;
                ((ID3D11Texture1D*)mpTexture)->GetDesc( &desc );
                desc.Usage = D3D11_USAGE_STAGING;
                switch( type )
                {
                case CPUT_MAP_READ:
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                    desc.BindFlags = 0;
                    break;
                case CPUT_MAP_READ_WRITE:
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
                    desc.BindFlags = 0;
                    break;
                case CPUT_MAP_WRITE:
                case CPUT_MAP_WRITE_DISCARD:
                case CPUT_MAP_NO_OVERWRITE:
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                    desc.BindFlags = 0;
                    break;
                };
                hr = pD3dDevice->CreateTexture1D( &desc, NULL, (ID3D11Texture1D**)&mpTextureStaging );
                ASSERT( SUCCEEDED(hr), _L("Failed to create staging texture") );
                break;
            }
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            {
                D3D11_TEXTURE2D_DESC desc;
                ((ID3D11Texture2D*)mpTexture)->GetDesc( &desc );
                desc.Usage = D3D11_USAGE_STAGING;
                switch( type )
                {
                case CPUT_MAP_READ:
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                    desc.BindFlags = 0;
                    break;
                case CPUT_MAP_READ_WRITE:
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
                    desc.BindFlags = 0;
                    break;
                case CPUT_MAP_WRITE:
                case CPUT_MAP_WRITE_DISCARD:
                case CPUT_MAP_NO_OVERWRITE:
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                    desc.BindFlags = 0;
                    break;
                };
                hr = pD3dDevice->CreateTexture2D( &desc, NULL, (ID3D11Texture2D**)&mpTextureStaging );
                ASSERT( SUCCEEDED(hr), _L("Failed to create staging texture") );
                break;
            }
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            {
                D3D11_TEXTURE3D_DESC desc;
                ((ID3D11Texture3D*)mpTexture)->GetDesc( &desc );
                desc.Usage = D3D11_USAGE_STAGING;
                switch( type )
                {
                case CPUT_MAP_READ:
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                    desc.BindFlags = 0;
                    break;
                case CPUT_MAP_READ_WRITE:
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
                    desc.BindFlags = 0;
                    break;
                case CPUT_MAP_WRITE:
                case CPUT_MAP_WRITE_DISCARD:
                case CPUT_MAP_NO_OVERWRITE:
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                    desc.BindFlags = 0;
                    break;
                };
                hr = pD3dDevice->CreateTexture3D( &desc, NULL, (ID3D11Texture3D**)&mpTextureStaging );
                ASSERT( SUCCEEDED(hr), _L("Failed to create staging texture") );
                break;
            }
        default:
            ASSERT(0, _L("Unkown texture dimension") );
            break;
        }
    }
    else
    {
        ASSERT( mMappedType == type, _L("Mapping with a different CPU access than creation parameter.") );
    }
    D3D11_MAPPED_SUBRESOURCE info;
    switch( type )
    {
    case CPUT_MAP_READ:
    case CPUT_MAP_READ_WRITE:
        // TODO: Copying and immediately mapping probably introduces a stall.
        // Expose the copy externally?
        // TODO: copy only if changed?
        // Copy only first time?
        // Copy the GPU version before we read from it.
        pContext->CopyResource( mpTextureStaging, mpTexture );
        break;
    };
    hr = pContext->Map( mpTextureStaging, wait ? 0 : D3D11_MAP_FLAG_DO_NOT_WAIT, (D3D11_MAP)type, 0, &info );
    mMappedType = type;
    return info;
} // CPUTTextureDX11::Map()

//-----------------------------------------------------------------------------
void CPUTTextureDX11::UnmapTexture( CPUTRenderParameters &params )
{
    ASSERT( mMappedType != CPUT_MAP_UNDEFINED, _L("Can't unmap a render target that isn't mapped.") );

    CPUTRenderParametersDX *pParamsDX11 = (CPUTRenderParametersDX*)&params;
    ID3D11DeviceContext *pContext = pParamsDX11->mpContext;

    pContext->Unmap( mpTextureStaging, 0 );

    // If we were mapped for write, then copy staging buffer to GPU
    switch( mMappedType )
    {
    case CPUT_MAP_READ:
        break;
    case CPUT_MAP_READ_WRITE:
    case CPUT_MAP_WRITE:
    case CPUT_MAP_WRITE_DISCARD:
    case CPUT_MAP_NO_OVERWRITE:
        pContext->CopyResource( mpTexture, mpTextureStaging );
        break;
    };
} // CPUTTextureDX11::Unmap()

