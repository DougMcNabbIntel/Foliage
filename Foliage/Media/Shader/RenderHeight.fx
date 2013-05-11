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

// ********************************************************************************************************
struct VS_INPUT
{
    float3 Pos      : POSITION;
};
struct PS_INPUT
{
    float4 Pos      : SV_POSITION;
};

// ********************************************************************************************************
cbuffer cbPerModelValues
{
    row_major float4x4 World : WORLD;
    row_major float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
    row_major float4x4 InverseWorld : INVERSEWORLD;
              float4   LightDirection;
              float4   EyePosition;
    row_major float4x4 LightWorldViewProjection;
};

// ********************************************************************************************************
cbuffer cbPerFrameValues
{
    row_major float4x4  View;
    row_major float4x4  Projection;
};

#if 0
// ********************************************************************************************************
PS_INPUT VSMain( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;

    float3 worldPos = mul(float4(input.Pos,1), World);
    output.Pos.xyz = worldPos.xzy * float3( 2.0f/5285.28, 2.0f/5285.28f, 1.0f/600.0f );

    // output.Pos.xyz = input.Pos * float3( 2.0f/5285.28, 2.0f/5285.28f, -1.0f/600.0f );
    output.Pos.w   = 1.0f;
    return output;
}
#endif

// ********************************************************************************************************
PS_INPUT VSMain( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;

    float3 worldPos = mul(float4(input.Pos,1), World);
    output.Pos.xy = worldPos.xz * 2/5280.0f; // Screen.xy = vert.xz / extents.
    output.Pos.z  = worldPos.y  * 1.0f/600.0f;
    output.Pos.w  = 1.0f;
    return output;
}


// ********************************************************************************************************
float4 PSMain( PS_INPUT input ) : SV_Target
{
    return float4(input.Pos.zzz, 1);
}
