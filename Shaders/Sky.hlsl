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
    
    // ���� ���� ��ġ�� �Թ�ü �� ��ȸ ���ͷ� ����Ѵ�.
    vout.PosL = vin.PosL;
    
    // ���� �������� ��ȯ�Ѵ�.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
    // �ϴ� ���� �߽��� �׻� ī�޶� ��ġ�� �д�.
    posW.xyz += gEyePosW;
    
    // z/w = 1�� �ǵ���(��, �ϴ� ���� �׻� �� ��鿡 �ֵ���) z = w�� �����Ѵ�.
    vout.PosH = mul(posW, gViewProj).xyww;
    
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float4 envColor = gCubeMap.Sample(gsamLinearWrap, pin.PosL);
    //envColor *= 0.5f; // �ϴ� ���� �ʹ� ��Ƽ� ���� ��Ӱ� �Ѵ�.`
    return envColor;
}