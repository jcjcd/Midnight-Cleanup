#include "commonMath.hlsl"
#include "PBRSupport.hlsl"
#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

cbuffer cbLightmapInformationV
{
    float4 gUVOffsetScale;
};

cbuffer cbLightmapInformationP
{
    uint gMapIndex;
    bool gUseLightmap;
    bool gUseDirectionMap;
};

cbuffer cbMaskBuffer
{
    int gRecieveDecal;
    int gReflect;
    int gRefract;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentL : TANGENT;
    float2 TexC : TEXCOORD0;
    float2 LightmapUV : TEXCOORD1;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD0;
    float ClipSpacePosZ : TEXCOORD1;
    float2 LightmapUV : TEXCOORD2;
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
Texture2D gEmissive;

Texture2DArray gLightMapArray;
Texture2DArray gDirectionArray;

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // 비균등 축소 확대가 있을수도 있기 때문에 월드의 역전치행렬을 사용한다..
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorldInvTranspose);
    vout.TangentW = mul(vin.TangentL, (float3x3) gWorld);
    
    // 노말을 정규화한다.
    vout.NormalW = normalize(vout.NormalW);
    vout.TangentW = normalize(vout.TangentW);

    // Transform to homogeneous clip space
    vout.PosH = mul(posW, gViewProj);

    // Output texture coordinates
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform).xy;

    // Clip space z
    vout.ClipSpacePosZ = vout.PosH.z;

    // Lightmap UV
    float2 lightmapUV = float2(vin.LightmapUV.x, 1 - vin.LightmapUV.y);
    lightmapUV = lightmapUV * gUVOffsetScale.zw + gUVOffsetScale.xy;
    lightmapUV.y = 1 - lightmapUV.y;
    vout.LightmapUV = lightmapUV;
    
    return vout;
}

PixelOut PSMain(VertexOut pin, bool isFrontFace : SV_IsFrontFace)
{
    PixelOut pout = (PixelOut) 0.0f;

    //pout.Albedo = gAlbedo.Sample(gsamAnisotropicWrap, pin.TexC);
    pout.Albedo = pow(gAlbedo.Sample(gsamAnisotropicWrap, pin.TexC), 2.2f);
    pout.Normal = gNormal.Sample(gsamAnisotropicWrap, pin.TexC);

    float3 normalW = NormalSampleToWorldSpace(pout.Normal.xyz, pin.NormalW, pin.TangentW);
    if (!isFrontFace)  // isFrontFace 값으로 앞뒤 판단
    {
        normalW = -normalW;
    }
    pout.Normal.xyz = normalW;

    //pout.Metallic = gORM.Sample(gsamAnisotropicWrap, pin.TexC).b;
    //pout.Roughness = gORM.Sample(gsamAnisotropicWrap, pin.TexC).g;
    //pout.AO = gORM.Sample(gsamAnisotropicWrap, pin.TexC).r;
    pout.ORM = gORM.Sample(gsamAnisotropicWrap, pin.TexC);
    pout.Emissive = gEmissive.Sample(gsamAnisotropicWrap, pin.TexC);
    pout.PositionW = float4(pin.PosW, pin.ClipSpacePosZ);
    
    pout.Mask.r = gRecieveDecal;
    pout.Mask.g = gReflect;
    pout.Mask.b = gRefract;

    // Lightmap
    if (gUseLightmap)
    {
        pout.Light = gLightMapArray.Sample(gsamLinearWrap, float3(pin.LightmapUV, gMapIndex));
        
        if (gUseDirectionMap)
        {
            float4 direction = gDirectionArray.Sample(gsamPointWrap, float3(pin.LightmapUV, gMapIndex));
            direction.xyz = direction.xyz * 2 - 1;
            //direction.x = -direction.x;
            //direction.z = -direction.z;
            //direction = normalize(direction);

            float halfLambert = dot(pout.Normal.xyz, direction.xyz) * 0.5 + 0.5;
            pout.Light = pout.Light * halfLambert / max(1e-4, direction.w);
            //float3 viewDir = normalize(gEyePosW - pin.PosW);
            //float3 halfDir = normalize(viewDir + direction.xyz);
            //float ndotl = saturate(dot(pout.Normal.xyz, direction.xyz));
            //float ndotv = saturate(dot(pout.Normal.xyz, viewDir));
            //float3 F0 = tofloat3(0.04f);
            //F0 = lerp(F0, pout.Albedo.xyz, pout.Metallic);

            // Specular
            //float3 F = fresnelSchlick(max(0.0, dot(halfDir, viewDir)), F0);  // 프레넬
            //float D = DistributionGGX(pout.Normal.xyz, halfDir, pout.Roughness); // 정반사 분포도
            //float G = GeometrySmith(pout.Normal.xyz, viewDir, direction.xyz, pout.Roughness); // 기하 감쇠율
            
            //float3 specular = F * G * D / max(0.00001f,4.0f * ndotl * ndotv);

            // Diffuse
            //float3 kD = lerp(tofloat3(1.0f)-F, tofloat3(0.0f), pout.Metallic);
            //float3 diffuse = (kD * pout.Albedo) / PI;

            //float3 resultColor = (diffuse+specular)*ndotl;
            //pout.Light.xyz = pout.Light.xyz * resultColor;
        }
    }
    else
    {
        pout.Light = float4(0.0f,0.0f,0.0f,-1000.0f);
    }

    return pout;
}
