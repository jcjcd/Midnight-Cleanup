// (C) 2019 David Lettier
// lettier.com

#include "SamplerStates.hlsl"

cbuffer LensProjection : register(b0)
{
    float4x4 lensProjection;
    float4x4 lensView;
    
    float2 texSize;
    float2 enabled;
    
    float3 lensPosition;
}

Texture2D positionTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D maskTexture : register(t2);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentL : TANGENT;
    float2 TexC : TEXCOORD0;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    vout.position = float4(vin.PosL, 1.0f);
	
    // Output texture coordinates
    vout.texCoord = vin.TexC;

    return vout;
}

float4 PSMain(VertexOut input) : SV_TARGET
{
    //float maxDistance = 8;
    //float resolution = 0.3;
    //int steps = 5;
    //float thickness = 0.5;

    //float maxDistance = 300;
    //float resolution = 0.8;
    //int steps = 10;
    //float thickness = 1;
    
    float maxDistance = 50.f;
    float resolution = 0.3;
    int steps = 10;
    float thickness = 0.05;


    float2 texCoord = input.texCoord;

    float4 uv = float4(0.0, 0.0, 0.0, 0.0);

    float4 positionOrigin = positionTexture.Sample(gsamLinearClamp, texCoord);
    positionOrigin.w = 1.0;
    float4 positionFrom = mul(positionOrigin, lensView); // mul(positionOrigin, lensView)
    float4 mask = maskTexture.Sample(gsamLinearClamp, texCoord);

    if (positionFrom.w <= 0.0 || enabled.x != 1.0 || mask.g <= 0.0)
    {
        return uv;
    }

    
    float3 unitPositionFrom = normalize(positionFrom.xyz);
    float3 normalWorld = normalize(normalTexture.Sample(gsamLinearClamp, texCoord).xyz);
    float3 normal = mul(normalWorld, (float3x3) lensView);
    normal = normalize(normal);
    
    float3 pivot = normalize(reflect(unitPositionFrom, normal));
    
    float4 positionTo = positionFrom;

    float4 startView = float4(positionFrom.xyz + (pivot * 0.0), 1.0);
    float4 endView = float4(positionFrom.xyz + (pivot * maxDistance), 1.0);

    //float4 startFrag = mul(lensProjection, startView);
    float4 startFrag = mul(startView, lensProjection); 
    startFrag.xyz /= startFrag.w;
    startFrag.xy = startFrag.xy * 0.5 + 0.5;
    startFrag.y = 1.0 - startFrag.y;
    startFrag.xy *= texSize;

    //float4 endFrag = mul(lensProjection, endView);
    float4 endFrag = mul(endView, lensProjection);
    endFrag.xyz /= endFrag.w;
    endFrag.xy = endFrag.xy * 0.5 + 0.5;
    endFrag.y = 1.0 - endFrag.y;
    endFrag.xy *= texSize;
    
    if (startFrag.z < 0.0 || startFrag.z > 1.0 || endFrag.z < 0.0 || endFrag.z > 1.0)
    {
        return float4(0.0, 0.0, 0.0, 0.0); // Return black pixel
    }

    float2 frag = startFrag.xy;
    uv.xy = saturate(frag / texSize); // Clamping texCoords

    float deltaX = endFrag.x - startFrag.x;
    float deltaY = endFrag.y - startFrag.y;
    float useX = abs(deltaX) >= abs(deltaY) ? 1.0 : 0.0;
    float delta = lerp(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0.0, 1.0);
    float2 increment = float2(deltaX, deltaY) / max(delta, 0.001);

    float search0 = 0;
    float search1 = 0;

    bool hit0 = false;
    bool hit1 = false;

    float viewDistance = startView.z;
    float depth = thickness;

    int maxIterations = 100; // Limit loop iterations
    int i = 0;

    // Use [loop] for more control over the loop
    [loop]
    for (i = 0; i < min(int(delta), maxIterations); ++i)
    {
        frag += increment;
        uv.xy = saturate(frag / texSize); // Clamping texCoords
        positionTo = positionTexture.Sample(gsamLinearClamp, uv.xy);
        positionTo.w = 1.f;
        positionTo = mul(positionTo, lensView);

        search1 = lerp((frag.y - startFrag.y) / deltaY, (frag.x - startFrag.x) / deltaX, useX);
        search1 = clamp(search1, 0.0, 1.0);

        viewDistance = (startView.z * endView.z) / lerp(endView.z, startView.z, search1);
        depth = viewDistance - positionTo.z;

        if (depth > 0.001 && depth < thickness + 0.001)
        {
            hit0 = true;
            break;
        }
        else
        {
            search0 = search1;
        }
    }

    search1 = search0 + ((search1 - search0) / 2.0);
    steps *= hit0 ? 1 : 0; // Only enter if hit0 is true

    // Second loop with max iterations check
    [loop]
    for (i = 0; i < min(steps, maxIterations); ++i)
    {
        frag = lerp(startFrag.xy, endFrag.xy, search1);
        uv.xy = saturate(frag / texSize); // Clamping texCoords
        positionTo = positionTexture.Sample(gsamLinearClamp, uv.xy);
        positionTo.w = 1.f;
        positionTo = mul(positionTo, lensView);

        viewDistance = (startView.z * endView.z) / lerp(endView.z, startView.z, search1);
        depth = viewDistance - positionTo.z;

        if (depth > 0.001 && depth < thickness + 0.001)
        {
            hit1 = true;
            search1 = search0 + ((search1 - search0) / 2);
        }
        else
        {
            float temp = search1;
            search1 = search1 + ((search1 - search0) / 2);
            search0 = temp;
        }
    }

    float visibility = hit1 * positionTo.w * (1 - max(dot(-unitPositionFrom, pivot), 0)) *
                       (1 - clamp(depth / thickness, 0, 1)) *
                       (1 - clamp(length(positionTo - positionFrom) / maxDistance, 0, 1)) *
                       (uv.x < 0 || uv.x > 1 ? 0 : 1) *
                       (uv.y < 0 || uv.y > 1 ? 0 : 1);

    visibility = clamp(visibility, 0, 1);

    uv.ba = float2(visibility, visibility);
    
    //if (visibility <= 0.0)
    //{
    //    uv.a = -1.f;
    //    float3 reflection = reflect(positionOrigin.xyz - lensPosition, normalWorld);
    //    uv.rgb = reflection;
    //}

    return uv;
}