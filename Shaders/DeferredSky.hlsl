#include "commonMath.hlsl"
#include "PBRSupport.hlsl"
#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD0;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float ClipSpacePosZ : TEXCOORD2;
};

struct PixelOut
{
    float4 Albedo : SV_TARGET0;
    //float4 ORM : SV_TARGET1;
    //float4 Normal : SV_TARGET2;
    float4 PositionW : SV_TARGET1;
    float4 Emissive : SV_TARGET2;
    //float4 Light : SV_TARGET5;
    //float4 Mask : SV_TARGET6;
    
};

TextureCube gCubeMap;


VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    
    // 국소 정점 위치를 입방체 맵 조회 벡터로 사용한다.
    vout.PosW = vin.PosL;
    
    // 세계 공간으로 변환한다.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
    // 하늘 구의 중심을 항상 카메라 위치에 둔다.
    posW.xyz += gEyePosW;
    
    // z/w = 1이 되도록(즉, 하늘 구가 항상 먼 평면에 있도록) z = w로 설정한다.
    vout.PosH = mul(posW, gViewProj).xyww;
   
    // 클립 공간 z 값을 저장한다.
    vout.ClipSpacePosZ = vout.PosH.z;
    
    return vout;
}

PixelOut PSMain(VertexOut pin, bool isFrontFace : SV_IsFrontFace)  
{
    PixelOut pout = (PixelOut) 0.0f;

    // 하늘 구를 그린다.
    float4 envColor = gCubeMap.Sample(gsamLinearWrap, pin.PosW);
    pout.PositionW = float4(pin.PosW, pin.ClipSpacePosZ);
    pout.Emissive = envColor;
    
    //pout.Mask.a = 1.0f;
    

    return pout;
}
