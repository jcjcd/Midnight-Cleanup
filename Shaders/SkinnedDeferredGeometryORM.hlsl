#include "commonMath.hlsl"
#include "PBRSupport.hlsl"
#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentL : TANGENT;
    float2 TexC : TEXCOORD0;
        
    int4 Indices : INDICES;
    float4 Weights : WEIGHTS;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD0;
    float ClipSpacePosZ : TEXCOORD1;
};

cbuffer BoneMatrixBuffer
{
    matrix gBoneTransforms[128];
};

cbuffer cbMaskBuffer
{
    int gRecieveDecal;
    int gReflect;
    int gRefract;
};

struct PixelOut
{
    float4 Albedo : SV_TARGET0;
    float4 ORM : SV_TARGET1;
    float4 Normal : SV_TARGET2;
    float4 PositionW : SV_TARGET3;
    float4 Emissive : SV_TARGET4;
    float4 Light : SV_TARGET5;
    float4 Mask : SV_TARGET6;
    
};

Texture2D gAlbedo;
Texture2D gNormal;
Texture2D gORM;

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    float4x4 worldMat;

    worldMat = mul(vin.Weights.x, gBoneTransforms[vin.Indices.x]);
    worldMat += mul(vin.Weights.y, gBoneTransforms[vin.Indices.y]);
    worldMat += mul(vin.Weights.z, gBoneTransforms[vin.Indices.z]);
    worldMat += mul(vin.Weights.w, gBoneTransforms[vin.Indices.w]);
    

    worldMat = mul(worldMat, gWorld);
    

    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), worldMat);
    vout.PosW = posW.xyz;

    // 비균등 축소 확대가 있을수도 있기 때문에 월드의 역전치행렬을 사용한다..
    // 사용하고싶다..
    vout.NormalW = mul(vin.NormalL, (float3x3)worldMat);
    vout.NormalW = normalize(vout.NormalW);
    vout.TangentW = mul(vin.TangentL, (float3x3)worldMat);
    vout.TangentW = normalize(vout.TangentW);

    // Transform to homogeneous clip space
    vout.PosH = mul(posW, gViewProj);

    // Output texture coordinates
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform).xy;
    vout.ClipSpacePosZ = vout.PosH.z;
    
    return vout;
}

PixelOut PSMain(VertexOut pin)
{
    PixelOut pout = (PixelOut) 0.0f;

    pout.Albedo = gAlbedo.Sample(gsamAnisotropicWrap, pin.TexC);
    pout.Normal = gNormal.Sample(gsamAnisotropicWrap, pin.TexC);

    float3 normalW = NormalSampleToWorldSpace(pout.Normal.xyz, pin.NormalW, pin.TangentW);
    pout.Normal.xyz = normalW;
    pout.ORM = gORM.Sample(gsamAnisotropicWrap, pin.TexC);
    pout.PositionW = float4(pin.PosW, pin.ClipSpacePosZ);
    
    pout.Mask.r = gRecieveDecal;
    pout.Mask.g = gReflect;
    pout.Mask.b = gRefract;


    return pout;
}
