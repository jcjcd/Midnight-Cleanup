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
    
    // ���� ���� ��ġ�� �Թ�ü �� ��ȸ ���ͷ� ����Ѵ�.
    vout.PosW = vin.PosL;
    
    // ���� �������� ��ȯ�Ѵ�.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
    // �ϴ� ���� �߽��� �׻� ī�޶� ��ġ�� �д�.
    posW.xyz += gEyePosW;
    
    // z/w = 1�� �ǵ���(��, �ϴ� ���� �׻� �� ��鿡 �ֵ���) z = w�� �����Ѵ�.
    vout.PosH = mul(posW, gViewProj).xyww;
   
    // Ŭ�� ���� z ���� �����Ѵ�.
    vout.ClipSpacePosZ = vout.PosH.z;
    
    return vout;
}

PixelOut PSMain(VertexOut pin, bool isFrontFace : SV_IsFrontFace)  
{
    PixelOut pout = (PixelOut) 0.0f;

    // �ϴ� ���� �׸���.
    float4 envColor = gCubeMap.Sample(gsamLinearWrap, pin.PosW);
    pout.PositionW = float4(pin.PosW, pin.ClipSpacePosZ);
    pout.Emissive = envColor;
    
    //pout.Mask.a = 1.0f;
    

    return pout;
}
