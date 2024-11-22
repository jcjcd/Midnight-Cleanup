#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentL : TANGENT;
    float2 TexC : TEXCOORD0;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

cbuffer cbFog
{
    float4 gFogColor;
    float gFogDensity;
    float gFogStart;
    float gFogRange;
    int bUseFog;
};

cbuffer cbBloom
{
    float4 gBloomColorTint;
    float gBloomThreshold;
    float gBloomIntensity;
    int bUseBloom;
    int gBloomCurveType;
};

cbuffer cbColorAdjust
{
    int bUseColorAdjust;
    float gExposure;
    float gContrast;
    float gSaturation;
};

float GetBloomCurve(float intensity, float threshold, int type)
{
    float result = intensity;
    intensity *= 2.0f;

    if(type == 0)
    {   
        result = intensity * 0.05f + max(0, intensity - threshold) * 0.5f;
    }
    else if(type == 1)
    {
        result = intensity * intensity / 3.2f;
    }
    else
    {
        result = max(0, intensity - threshold);
        result *= result;
    }

    return result * 0.5f;
}

float GetBrightness(float3 color)
{
    return dot(color.rgb, float3(0.299, 0.587, 0.114));
}

float3 AdjustExposure(float3 color, float exposure)
{
    exposure = pow(exposure, 2.f);
    color = saturate(color.rgb * exposure);
    //color.rgb *= exposure;
    return color;
}

float3 AdjustContrast(float3 color, float contrast)
{
    contrast = contrast * 0.01f + 1.f;
    float brightness = GetBrightness(color); // °¢ ÇÈ¼¿ÀÇ Æò±Õ ¹à±â ±â¹Ý
    float3 mean = float3(brightness, brightness, brightness); // Æò±Õ ¹à±â °ª
    color.rgb = (color.rgb - mean) * contrast + mean;
    color = saturate(color);
    return color;
}

float3 AdjustSaturation(float3 color, float saturation)
{
    saturation = saturation * 0.01f + 1.f;
    float3 gray = GetBrightness(color); // È¸»öÁ¶ °ª
    color.rgb = lerp(gray, color.rgb, saturation);
    color = saturate(color);
    return color;
}

Texture2D gSrcTexture;
Texture2D gBloomTexture;
Texture2D gPositionW;

VertexOut VSMain(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
	
    // Output texture coordinates
    vout.TexC = vin.TexC;

    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float3 litColor = gSrcTexture.Sample(gsamPointClamp, pin.TexC).rgb;

    if(bUseBloom)
    {
        float3 bloomColor = gBloomTexture.Sample(gsamPointClamp, pin.TexC).rgb;
        litColor += bloomColor * GetBloomCurve(gBloomIntensity, gBloomThreshold, gBloomCurveType) * gBloomColorTint.rgb;
    }

    if(bUseFog)
    {
        float depth = gPositionW.Sample(gsamPointClamp, pin.TexC).w;
        float fogFactor = saturate((depth - gFogStart) / gFogRange);
        litColor = lerp(litColor, gFogColor.xyz, fogFactor);
    }

    if(bUseColorAdjust)
    {
        litColor = min(litColor, 60.0f);
        litColor = AdjustExposure(litColor, gExposure);
        litColor = AdjustContrast(litColor, gContrast);
        litColor = max(litColor, 0.0f);
        litColor = AdjustSaturation(litColor, gSaturation);
        litColor /= litColor + 1.0f;
    }
    
    return float4(litColor, 1.0f);
}


