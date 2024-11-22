// (C) 2019 David Lettier
// lettier.com

#include "SamplerStates.hlsl"

cbuffer LensProjection : register(b0)
{
    float4x4 lensProjection;
    float4x4 lensView;
    
    float2 texSize;
    float2 enabled;
    float2 rior;
    
}

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

Texture2D positionFromTexture : register(t0);
Texture2D positionToTexture : register(t1);
Texture2D normalFromTexture : register(t2);

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
    
    float maxDistance = 150.0f;
    float resolution = 0.5f;
    int steps = 10;
    float thickness = 1.0f;

    float2 texCoord = input.texCoord;

    float4 uv = float4(texCoord.xy, 1.0f, 1.0f);

    //float4 positionFrom = positionFromTexture.Sample(gsamLinearClamp, texCoord);
    float4 positionOrigin = positionFromTexture.Sample(gsamLinearClamp, texCoord);
    positionOrigin.w = 1.0;
    float4 positionFrom = mul(positionOrigin, lensView); // mul(positionOrigin, lensView)

    if (positionFrom.w <= 0.0f || enabled.x != 1.0f)
    {
        return uv;
    }

    float3 unitPositionFrom = normalize(positionFrom.xyz);
    
    //float3 normalFrom = normalize(normalFromTexture.Sample(gsamLinearClamp, texCoord).xyz);
    float3 normalFrom = normalize(normalFromTexture.Sample(gsamLinearClamp, texCoord).xyz);
    normalFrom = mul(normalFrom, (float3x3)lensView);
    normalFrom = normalize(normalFrom);
    
    float3 pivot = normalize(refract(unitPositionFrom, normalFrom, rior.x));

    float4 positionTo = positionFrom;

    float4 startView = float4(positionFrom.xyz + (pivot * 0.0f), 1.0f);
    float4 endView = float4(positionFrom.xyz + (pivot * maxDistance), 1.0f);

    float4 startFrag = mul(startView, lensProjection);
    startFrag.xyz /= startFrag.w;
    startFrag.xy = startFrag.xy * 0.5f + 0.5f;
    startFrag.y = 1.0 - startFrag.y;
    startFrag.xy *= texSize;

    float4 endFrag = mul(endView, lensProjection);
    endFrag.xyz /= endFrag.w;
    endFrag.xy = endFrag.xy * 0.5f + 0.5f;
    endFrag.y = 1.0 - endFrag.y;
    endFrag.xy *= texSize;

    float2 frag = startFrag.xy;
    uv.xy = frag / texSize;

    float deltaX = endFrag.x - startFrag.x;
    float deltaY = endFrag.y - startFrag.y;
    float useX = abs(deltaX) >= abs(deltaY) ? 1.0f : 0.0f;
    float delta = lerp(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0.0f, 1.0f);
    float2 increment = float2(deltaX, deltaY) / max(delta, 0.001f);

    float search0 = 0.0f;
    float search1 = 0.0f;

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
        uv.xy = frag / texSize;
        positionTo = positionToTexture.Sample(gsamLinearClamp, uv.xy);
        positionTo.w = 1.f;
        positionTo = mul(positionTo, lensView);
        
        search1 = lerp((frag.y - startFrag.y) / deltaY, (frag.x - startFrag.x) / deltaX, useX);
        search1 = clamp(search1, 0.0f, 1.0f);

        viewDistance = (startView.z * endView.z) / lerp(endView.z, startView.z, search1);
        depth = viewDistance - positionTo.z;

        if (depth > 0.0f && depth < thickness)
        {
            hit0 = true;
            break;
        }
        else
        {
            search0 = search1;
        }
    }

    search1 = search0 + ((search1 - search0) / 2.0f);
    steps *= hit0 ? 1 : 0;
   
    // Use [loop] for more control over the loop
    [loop]
    for (i = 0; i < min(steps, maxIterations); ++i)
    {
        frag = lerp(startFrag.xy, endFrag.xy, search1);
        uv.xy = frag / texSize;
        positionTo = positionToTexture.Sample(gsamLinearClamp, uv.xy);
        positionTo.w = 1.f;
        positionTo = mul(positionTo, lensView);

        viewDistance = (startView.z * endView.z) / lerp(endView.z, startView.z, search1);
        depth = viewDistance - positionTo.z;

        if (depth > 0.0f && depth < thickness)
        {
            hit1 = true;
            search1 = search0 + ((search1 - search0) / 2.0f);
        }
        else
        {
            float temp = search1;
            search1 = search1 + ((search1 - search0) / 2.0f);
            search0 = temp;
        }
    }

    float visibility = hit1 * positionTo.w * (1.0f - max(dot(-unitPositionFrom, pivot), 0.0f)) *
                       (uv.x < 0.0f || uv.x > 1.0f ? 0.0f : 1.0f) *
                       (uv.y < 0.0f || uv.y > 1.0f ? 0.0f : 1.0f);

    visibility = clamp(visibility, 0.0f, 1.0f);

    return float4(lerp(texCoord.xy, uv.xy, visibility), 1.0f, 1.0f);
}
