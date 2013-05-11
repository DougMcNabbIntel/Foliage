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
cbuffer cbPerModelValues
{
    row_major float4x4 World : WORLD;
    row_major float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
    row_major float4x4 InverseWorld : INVERSEWORLD;
              float4   LightDirection;
              float4   EyePosition;
    row_major float4x4 LightWorldViewProjection;

    row_major float4x4 ViewProjection;
              float4   BoundingBoxCenterWorldSpace;
              float4   BoundingBoxHalfWorldSpace;
              float4   BoundingBoxCenterObjectSpace;
              float4   BoundingBoxHalfObjectSpace;
};
// -------------------------------------
cbuffer cbPerFrameValues
{
    row_major float4x4  View;
    row_major float4x4  Projection;
              float3    AmbientColor;
              float3    LightColor;
              float3    TotalTimeInSeconds;
    row_major float4x4  InverseView;
};
// ********************************************************************************************************
// Define an empty structure for declaring GS input.
// We don't have a VB (i.e. implicit data).  Instead we explicitly fetch from a buffer.
// Using the empty structure elliminates a DX runtime warning.
struct EMPTY
{
};
// ********************************************************************************************************
struct VS_INPUT
{
    float3 Position     : POSITION;
    uint   TextureIndex : TEXCOORD0;
    float2 Scale        : TEXCOORD1;
    float4 UVExtent     : TEXCOORD2;
};
// ********************************************************************************************************
struct PS_INPUT
{
    float4 Position  : SV_POSITION;
    float3 Uvw       : TEXCOORD0;
    float4 Color     : TEXCOORD1;
    float  FogAmount : TEXCOORD2;
    float  AmbientOcclusion : TEXCOORD3;
};
// ********************************************************************************************************
struct Billboard
{
    float3 Position;
    uint   TextureIndex;
    float2 Scale;
    float4 UVExtent;
    float2 Offset;
};
// ********************************************************************************************************
Texture2DArray  BillboardTexture;
Texture2DArray  NormalTexture;
Texture2D       TerrainLightMap;

SamplerState           SAMPLER0       : register(s0);

StructuredBuffer<Billboard>  BillboardBuffer;

// ********************************************************************************************************
VS_INPUT VSMain( VS_INPUT input )
{
    VS_INPUT output = (VS_INPUT)0;
    output.Position     = input.Position;
    output.TextureIndex = input.TextureIndex;
    output.Scale        = input.Scale;
    output.UVExtent     = input.UVExtent;
    return output;
}

// ********************************************************************************************************
// * Process 2X indices.  If odd, then left side, otherwise right side.
int SortIndex( int index )
{
    // Pseudo-sort the plants wrt distance along look
    float3 look        = View._m02_m12_m22;
    float2 eyePosition = InverseView._m30_m32; // == .xz

    // Repeat each index, once for left side, and once for right side
    bool leftRight = index & 1;
    index >>= 1; // /= 2

    // Determine X and Y from index
    int inX = index & 0x7F; // i.e., % 128; // TODO: 128 must match grid width
    int inY = index >> 7;

    // TODO: Is there a simpler expression for the following?
    int xx, yy, sortedIndex;
    if( abs(look.x) > abs(look.z) ) {
        if(   look.x > 0.0f ) { xx = 127-inY; yy = leftRight ?     inX : 127-inX; } // East
        else                  { xx =     inY; yy = leftRight ? 127-inX :     inX; } // West
    } else {
        if(   look.z > 0.0f ) { yy = 127-inY; xx = leftRight ? 127-inX :     inX; } // North
        else                  { yy =     inY; xx = leftRight ?     inX : 127-inX; } // South
    }
    sortedIndex = yy * 128 + xx;

    Billboard billboard  = BillboardBuffer[sortedIndex];
    float2 plantPosition = billboard.Position.xz;

    float2 lookNormal    = float2( -look.z, look.x ); // look rotated 90 degress around up
    float2 cameraToPlant = plantPosition - eyePosition;
    bool   side          = (0 >= (dot(lookNormal, cameraToPlant)));
    if( side == leftRight )
    {
        return sortedIndex;
    }
    return -1;
}

// ********************************************************************************************************
[maxvertexcount(4)]
void GSMain(
    point EMPTY input[1],
    inout TriangleStream<PS_INPUT> triStream,
    uint index : SV_PrimitiveID // Note: our primitive is a point, so same as SV_VertexID which isn't available to GS.
){
    int sortedIndex = SortIndex(index);
    if( sortedIndex == -1 ) return;
    Billboard billboard = BillboardBuffer[sortedIndex];

    int textureIndex = billboard.TextureIndex;
    if( textureIndex == -1 )
    {
        // This plant is disabled.  Skip it.
        return;
    }
    const float farFadeStart[MAX_PLANT_TYPES]        = {FAR_FADE_START};
    const float farFadeEnd[MAX_PLANT_TYPES]          = {FAR_FADE_END};
    const float farFadeDenominator[MAX_PLANT_TYPES]  = {FAR_FADE_DENOMINATOR};

    const float nearFadeStart[MAX_PLANT_TYPES]       = {NEAR_FADE_START};
    const float nearFadeEnd[MAX_PLANT_TYPES]         = {NEAR_FADE_END};
    const float nearFadeDenominator[MAX_PLANT_TYPES] = {NEAR_FADE_DENOMINATOR};

    const float3 eyePosition = InverseView._m30_m31_m32;

    // Output four vertices
    PS_INPUT output = (PS_INPUT)0;

    float4 position     = float4( billboard.Position, 1.0f );
    float4 basePosition = float4( billboard.Position, 1.0f);

    // Fetch the light/shadow amount from the light maps
    float2 lightMapUv = position.xz * 1.0/5280.0f + 0.5f;

    // Fetch shadow/light map.  Green channel is terrain shadow only.  Red is terrain and tree shadows.
    // TODO: need better way to specify which on each plant uses (e.g., small plants receive tree shadows, trees don't)
    float3 terrainLightMap = TerrainLightMap.SampleLevel( SAMPLER0, lightMapUv, 0 );
    float  lightMap = textureIndex < 3 ? terrainLightMap.r : terrainLightMap.g;
    output.AmbientOcclusion = textureIndex < 3 ? terrainLightMap.b : 1.0f;

    float3 eyeToPosition   = position.xyz - eyePosition;

    float uLeft   = billboard.UVExtent.x;
    float uRight  = billboard.UVExtent.y;
    float vTop    = billboard.UVExtent.z;
    float vBottom = billboard.UVExtent.w;

    float3 up    = float3(0,1,0);
    float2 scale = billboard.Scale;
    scale.x *= 0.5f; // Plant height set from 0 to scale.y, but plant width set from -scale.x to scale.x (== range of 2X).  Multiply by 0.5 to set range to 1X.

    float3 look = View._m02_m12_m22;
    float3 right = normalize(cross( -look, up )); // We have camera look.  Want plant's look.  plant.look == -camera.look.
    float3 scaledRight = right * scale.x;

    basePosition.xz += right.xz * billboard.Offset.x;
    basePosition.y  += billboard.Offset.y; // TODO: Move to CPU/Patch-init code

    position        = float4(basePosition.xyz - scaledRight, 1.0f);
    output.Position = mul( position, WorldViewProjection );
    output.Uvw      = float3( uRight, vTop, textureIndex );

    // Compute alpha based on near and far fade distances.
    float distanceEyeToPosition = length(eyeToPosition);
    // float farAlpha  = 1.0f - saturate((distanceEyeToPosition-farFadeStart[textureIndex])/(farFadeEnd[textureIndex]-farFadeStart[textureIndex]));
    // float nearAlpha = 1.0f - saturate((distanceEyeToPosition-nearFadeStart[textureIndex])/(nearFadeEnd[textureIndex]-nearFadeStart[textureIndex]));
    float farAlpha  = 1.0f - saturate((distanceEyeToPosition-farFadeStart[textureIndex])  * farFadeDenominator[textureIndex]);
    float nearAlpha = 1.0f - saturate((distanceEyeToPosition-nearFadeStart[textureIndex]) * nearFadeDenominator[textureIndex]);

    float alpha = min(farAlpha, nearAlpha);

    if( alpha <= 0.01 )
    {
        return;  // Don't spend time drawing this if it is almost completely transparent
    }

    // Output lightmap value for downstream tint.
    output.Color    = float4(lightMap.xxx, alpha);

    // TODO: Move these values to the program and pass in as defines.
    output.FogAmount = saturate((distanceEyeToPosition-FOG_START)/(FOG_END-FOG_START));
    triStream.Append(output);

    // Compute the next three of this billboard's vertices , reusing values from the first vertex when possible.

    position        = float4(basePosition.xyz + scaledRight, 1.0f);
    output.Position = mul( position, WorldViewProjection );
    output.Uvw      = float3( uLeft, vTop, textureIndex );
    // *** Reuse color, fog, and alpha from first vertex.  Close enough.
    triStream.Append(output);

    position        = float4(basePosition.xyz - right  * scale.x + up * scale.y, 1.0f);
    output.Position = mul( position, WorldViewProjection );
    output.Uvw      = float3( uRight, vBottom, textureIndex );
    // *** Reuse color, fog, and alpha from first vertex.  Close enough.
    triStream.Append(output);

    position        = float4(basePosition.xyz + right  * scale.x + up * scale.y, 1.0f);
    output.Position = mul( position, WorldViewProjection );
    output.Uvw      = float3( uLeft, vBottom, textureIndex );
    // *** Reuse color, fog, and alpha from first vertex.  Close enough.
    triStream.Append(output);

    // Not required.  Restart implied for every GS invocation
    // triStream.RestartStrip();
}

// ********************************************************************************************************
float4 PSMain( PS_INPUT input ) : SV_Target
{
    float4 diffuseTexture = BillboardTexture.Sample( SAMPLER0, input.Uvw );
    diffuseTexture.rgb /= diffuseTexture.a; // undo pre-multipy

    float baseAlpha = diffuseTexture.a;

    // Determine alpha, and perform "alpha test"
    float4 result;
    clip( baseAlpha - 0.02f ); // aka "Alpha test"
    if( baseAlpha < 0.02 ) { return float4(0,0,0,0); } // Skip the rest of the shader if pixel clipped
    result.a = baseAlpha;

    float3 normal = NormalTexture.Sample( SAMPLER0, input.Uvw ).xzy;
    normal        = normalize(normal * 2.0f - 1.0f);

    // TODO: calc view-space light-direction on host.  Pass via per-frame const buffer
    float3 viewSpaceLightDir = -mul( LightDirection, View ).xyz;

    float nDotL = 0.7 * dot( normal, viewSpaceLightDir );
    float backLight = 0.3 * saturate(-nDotL);
    nDotL = saturate(nDotL);

    // float3 ambientColor = lerp( 0.75 * AmbientColor, AmbientColor, input.AmbientOcclusion ); // Fuzzy shadow beneath trees
    float3 ambientColor = AmbientColor;
    float3 diffuse = ((LightColor * input.Color) * (nDotL + backLight) + ambientColor) * diffuseTexture.rgb;

    result.rgb = diffuse;
    result.a *= input.Color.a;

    result.xyz = lerp( result.rgb, FOG_COLOR, input.FogAmount );

    // Pre-multiply color by alpha
    result.rgb *= result.a;

    return result;
}

