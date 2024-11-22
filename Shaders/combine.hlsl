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
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};


Texture2D originTexture;
//Texture2D refractionTexture;
Texture2D reflectionTexture;


VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    vout.PosH = float4(vin.PosL, 1.0f);
	
    // Output texture coordinates
    vout.TexC = vin.TexC;

    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float2 texCoord = pin.TexC;
    float4 originColor = originTexture.Sample(gsamLinearClamp, texCoord);
    
    //float4 refractionColor = refractionTexture.Sample(gsamLinearClamp, texCoord);

    float4 reflectionColor = reflectionTexture.Sample(gsamLinearClamp, texCoord);
    
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    //finalColor.xyz = lerp(originColor.xyz, refractionColor.xyz, refractionColor.a);
    finalColor.xyz = lerp(originColor.xyz, reflectionColor.xyz, reflectionColor.a);
    
   
    return finalColor;
    
}