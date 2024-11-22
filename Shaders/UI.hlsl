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
	float2 TexC    : TEXCOORD;
};

cbuffer cbUIVertex
{
    float layerDepth;
    int is3D;
    float2 padding1;
};

cbuffer cbUIPixel
{
    float4 gColor;
    int isOn;
    float maskProgress; // 0 ~ 1
    int maskMode; // 1: Circular, 2: Rectangular
    float padding2;
};

// 원형 마스크 함수
float CircularMask(float2 uv, float progress)
{
    if (progress == 1.0f)
        return 1.0f;

    // UV 중심을 (0,0)으로 설정 (필요시 uv 좌표 조정)
    float2 centeredUV = uv - 0.5f;

    // 벡터 (x, y)에서 atan2(y, x) 사용
    float theta = atan2(centeredUV.x, centeredUV.y); // 중심을 기준으로 각도 계산

    theta += 3.141592655f; // PI를 더해서 0 ~ 2*PI 범위로 변환

    // 진행률에 따라 각도를 조절하여 시계방향으로 점점 사라지게 함
    float cutoff = progress * 6.28318531f; // progress * 2*PI

    // theta가 cutoff보다 크면 0, 작으면 1
    return step(theta, cutoff);
}

float HorizontalMask(float2 uv, float progress)
{
    // UV의 x 좌표를 기준으로 진행률을 계산
    return step(uv.x, progress);
}

Texture2D gDiffuseMap;

#define MASK_MODE_CIRCULAR 1
#define MASK_MODE_HORIZONTAL 2

VertexOut VSMain(VertexIn vin)
{
	VertexOut vout;
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(vout.PosH, gViewProj);
    vout.TexC = vin.TexC;

    if(is3D == 0)
        vout.PosH.z = layerDepth;
	
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float4 litColor = gDiffuseMap.Sample(gsamPointWrap, pin.TexC);
    litColor *= gColor;

    // 마스크 적용
    float mask = 1.0f;
    switch (maskMode)
    {
    case MASK_MODE_CIRCULAR:
        mask = CircularMask(pin.TexC, maskProgress);
        break;
    case MASK_MODE_HORIZONTAL:
        mask = HorizontalMask(pin.TexC, maskProgress);
        break;
    }
    litColor.a *= mask;

    if (isOn == 1)
        clip(litColor.a - 0.001f);
    else
        litColor.a = 0.0f;

    return litColor;
}


