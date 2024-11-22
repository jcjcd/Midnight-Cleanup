#include "particleCommon.hlsli"
#include "SamplerStates.hlsl"
#include "ShaderVariables.hlsl"

#define COLOR_MODE_MULTIPLY 0
#define COLOR_MODE_ADDITIVE 1
#define COLOR_MODE_SUBTRACT 2
#define COLOR_MODE_OVERLAY 3
#define COLOR_MODE_COLOR 4
#define COLOR_MODE_DIFFERENCE 5

struct VertexOut
{
    float3 PosW : POSITION;
    float4 Color : COLOR;
    float4 Age_LifeTime_Rotation_Size : TEXCOORD0;
};

struct PixelIn
{
    float4 PosH : SV_Position;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD0;
    float4 PosTex : TEXCOORD1;
};

static const matrix texTransform = 
{
    0.5f, 0.0f, 0.0f, 0.0f,
	0.0f, -0.5f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, 0.0f, 1.0f
};

StructuredBuffer<Particle> gParticleBuffer;
StructuredBuffer<uint> gIndexBuffer;

Texture2D gAlbedoTexture;
Texture2D gEmissiveTexture;
Texture2D gDepthTexture;

VertexOut VSMain(uint VertexID : SV_VertexID)
{
    VertexOut output = (VertexOut) 0;

    uint index = gIndexBuffer[VertexID];
    Particle pa = gParticleBuffer[index];

    output.PosW = pa.PositionW;
    output.Color = pa.Color;
    output.Age_LifeTime_Rotation_Size = float4(pa.Age, pa.LifeTime, pa.Rotation, pa.Size);

    return output;
}

[maxvertexcount(4)]
void GSMain(point VertexOut input[1], inout TriangleStream<PixelIn> SpriteStream)
{
    PixelIn output = (PixelIn) 0;
    
    const float2 offsets[4] =
    {
        float2(-1, 1),
        float2(1, 1),
        float2(-1, -1),
        float2(1, -1)
    };

    [unroll]
    for (int i = 0; i < 4;i++)
    {
        float2 offset = offsets[i];
        float2 uv = (offset + float2(1, 1)) * float2(0.5, 0.5);
        
        float radius = input[0].Age_LifeTime_Rotation_Size.w;
        
        float s, c;
        sincos(input[0].Age_LifeTime_Rotation_Size.z, s, c);
        float2x2 rotation = { float2(c, -s), float2(s, c) };
        
        offset = mul(offset, rotation);
        
        output.PosH = mul(float4(input[0].PosW, 1.f), gView);
        output.PosH.xy += radius * offset;
        output.PosH = mul(output.PosH, gProj);
        
        output.PosTex = mul(output.PosH, texTransform);
        
        output.Tex = mul(float4(uv, 0, 1), gParticleRender.TexTransform);
        output.Color = input[0].Color;
        
        SpriteStream.Append(output);
    }
    SpriteStream.RestartStrip();
}

float4 PSMain(PixelIn pin) : SV_Target
{
    pin.PosTex /= pin.PosTex.w;
    float depth = gDepthTexture.Sample(gsamLinearClamp, pin.PosTex.xy).x;
    
    if(pin.PosH.z > depth)
        clip(-1);

    
    float4 albedo = gParticleRender.BaseColor;
    
    if(gParticleRender.UseAlbedo)
        albedo *= gAlbedoTexture.Sample(gsamLinearClamp, pin.Tex,0);

    float3 emissive = gParticleRender.EmissiveColor.rgb;

    if(gParticleRender.UseEmissive)
        emissive *= gEmissiveTexture.Sample(gsamLinearClamp, pin.Tex,0).rgb;
    
    switch (gParticleRender.ColorMode)
    {
        case COLOR_MODE_MULTIPLY:
            albedo *= pin.Color;
            break;
        case COLOR_MODE_ADDITIVE:
            albedo += pin.Color;
            break;
        case COLOR_MODE_SUBTRACT:
            albedo -= pin.Color;
            break;
        case COLOR_MODE_OVERLAY:
            albedo = OverlayMode(albedo, pin.Color);
            break;
        case COLOR_MODE_COLOR:
            albedo = ColorMode(albedo, pin.Color);
            break;
        case COLOR_MODE_DIFFERENCE:
            albedo = abs(albedo - pin.Color);
            break;
    }

    float4 color = float4(albedo.rgb + emissive, albedo.a);

    if (gParticleRender.UseMultiplyAlpha)
        color.rgb *= color.a;

    clip(color.a - gParticleRender.AlphaCutOff);    

    return color;
}