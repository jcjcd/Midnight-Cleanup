#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

struct VertexIn
{ 
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

TextureCube gCubeMap;

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    
    // 국소 정점 위치를 입방체 맵 조회 벡터로 사용한다.
    vout.PosL = vin.PosL;
    
    // 세계 공간으로 변환한다.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
    // 하늘 구의 중심을 항상 카메라 위치에 둔다.
    posW.xyz += gEyePosW;
    
    // z/w = 1이 되도록(즉, 하늘 구가 항상 먼 평면에 있도록) z = w로 설정한다.
    vout.PosH = mul(posW, gViewProj).xyww;
    
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float4 envColor = gCubeMap.Sample(gsamLinearWrap, pin.PosL);
    //envColor *= 0.5f; // 하늘 구가 너무 밝아서 조금 어둡게 한다.`
    return envColor;
}