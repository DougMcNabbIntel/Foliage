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

SamplerComparisonState SHADOW_SAMPLER : register(s1);
Texture2D _Shadow;

// ********************************************************************************************************
struct VS_INPUT
{
    float3 Position : POSITION;
    float2 UV0      : TEXCOORD0;
};
struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 LightUV  : TEXCOORD0;
};

// -------------------------------------
float ComputeShadowAmount( PS_INPUT input )
{
    float3  lightUV = input.LightUV.xyz / input.LightUV.w;
    lightUV.xy = lightUV.xy * 0.5f + 0.5f; // TODO: Move to matrix?
    lightUV.y  = 1.0f - lightUV.y;
    float  shadowAmount = _Shadow.SampleCmp( SHADOW_SAMPLER, lightUV, lightUV.z ).x;
    return shadowAmount;
}

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

// ********************************************************************************************************
PS_INPUT VSMain( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;

    output.Position.xy = input.UV0 * 2.0f - 1.0f;
    output.Position.z  = 0.5f;
    output.Position.w  = 1.0f;
    output.LightUV = mul( float4(input.Position, 1.0f), LightWorldViewProjection );
    return output;
}

// ********************************************************************************************************
float4 PSMain( PS_INPUT input ) : SV_Target
{
    float  shadowAmount = ComputeShadowAmount(input);
    return float4(shadowAmount.xxx, 1);
}
