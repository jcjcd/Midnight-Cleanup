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
};

//struct VertexOut
//{
//    float4 PosH : SV_POSITION;
//    float3 PosW : POSITION;
//    float3 NormalW : NORMAL;
//    float3 TangentW : TANGENT;
//    float2 TexC : TEXCOORD0;
//    float ClipSpacePosZ : TEXCOORD1;
//};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 ReflectionMapSamplingPos : TEXCOORD0;
    float2 BumpMapSamplingPos : TEXCOORD1;
    float4 RefractionMapSamplingPos : TEXCOORD2;
    float2 BumpMapSamplingPos2 : TEXCOORD3;
    float4 PosW : POSITION;
    float ClipSpacePosZ : TEXCOORD4;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
};

struct PixelOut
{
    float4 Albedo : SV_TARGET0;
};

cbuffer cbWaterMatrix
{
    float4x4 gReflectionView;
    float4x4 gWindDirection;
    
    float gWaveLength;
    float gWaveHeight;
    
    float gTime;
    float gWindForce;
    float gDrawMode;
    
    int gFresnelMode;
    
    float gSpecPerturb;
    float gSpecPower;
    
    float gDullBlendFactor;
    
    int gEnableTextureBlending;
};

cbuffer cbMaskBuffer
{
    int gRecieveDecal;
    int gReflect;
    int gRefract;
};

TextureCube gReflectionMap : register(t0);
Texture2D RefractionMap : register(t1);
Texture2D NormalMap : register(t2);
Texture2D NormalMap2 : register(t3);

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW;
    
    float4x4 RVP = mul(gReflectionView, gProj);
    float4x4 WRVP = mul(gWorld, RVP);


    // Transform to homogeneous clip space
    
    vout.PosH = mul(posW, gViewProj);
    
    vout.ReflectionMapSamplingPos = mul(float4(vin.PosL, 1.0f), WRVP);
    //vout.PosH = mul(float4(vin.PosL, 1.0f), WRVP);
    vout.RefractionMapSamplingPos = mul(posW, gViewProj);

        // 비균등 축소 확대가 있을수도 있기 때문에 월드의 역전치행렬을 사용한다..
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorldInvTranspose);
    vout.TangentW = mul(vin.TangentL, (float3x3) gWorld);
    
    // 노말을 정규화한다.
    vout.NormalW = normalize(vout.NormalW);
    vout.TangentW = normalize(vout.TangentW);


    // Clip space z
    vout.ClipSpacePosZ = vout.PosH.z;
    
    
    float4 absoluteTexCoords = float4(vin.TexC, 0, 1);
    //float4 rotatedTexCoords = mul(absoluteTexCoords, gWindDirection);
    float4 rotatedTexCoords = absoluteTexCoords;
    float2 moveVector = float2(0, 1);
    float2 moveVector2 = float2(1, 0);
     
     // moving the water
    vout.BumpMapSamplingPos = rotatedTexCoords.xy / gWaveLength + gTime * gWindForce * moveVector.xy;
    //vout.BumpMapSamplingPos = absoluteTexCoords.xy;
    vout.BumpMapSamplingPos2 = rotatedTexCoords.xy / gWaveLength + gTime * gWindForce * moveVector2.xy;
    
    return vout;
}

PixelOut PSMain(VertexOut pin, bool isFrontFace : SV_IsFrontFace)  // SV_IsFrontFace를 픽셀 셰이더의 입력으로 받음
{
    PixelOut pout = (PixelOut) 0.0f;

    //float2 ProjectedTexCoords;
    //ProjectedTexCoords.x = pin.ReflectionMapSamplingPos.x / pin.ReflectionMapSamplingPos.w / 2.0f + 0.5f;
    //ProjectedTexCoords.y = -pin.ReflectionMapSamplingPos.y / pin.ReflectionMapSamplingPos.w / 2.0f + 0.5f;
    //float2 perturbatedTexCoords = ProjectedTexCoords + perturbation;

    float3 incident = -normalize(gEyePosW - pin.PosW.xyz);
    
    // sampling the bump map
    float4 bumpColor = NormalMap.Sample(gsamLinearWrap, pin.BumpMapSamplingPos);
    float4 bumpColor2 = NormalMap2.Sample(gsamLinearWrap, pin.BumpMapSamplingPos2);
    
    float3 normalW = NormalSampleToWorldSpace(bumpColor.rgb, pin.NormalW, pin.TangentW);
    float3 normalW2 = NormalSampleToWorldSpace(bumpColor2.rgb, pin.NormalW, pin.TangentW);
    
    normalW = normalize(normalW + normalW2);
    
    if (!isFrontFace)  // isFrontFace 값으로 앞뒤 판단
    {
        normalW = -normalW;
    }
    
    float3 reflectionVector = reflect(incident, normalW);
    
    float2 calcNormal = float2(normalW.r, normalW.g - 1.0f);
    
    
    
    // perturbating the color
    float2 perturbation = gWaveHeight * calcNormal;
    //float2 perturbation = float2(0, 0);
    
    // the final texture coordinates
    //float2 perturbatedTexCoords = ProjectedTexCoords;
    float4 reflectiveColor = gReflectionMap.Sample(gsamLinearWrap, reflectionVector);

    float2 ProjectedRefrTexCoords;
    ProjectedRefrTexCoords.x = pin.RefractionMapSamplingPos.x / pin.RefractionMapSamplingPos.w / 2.0f + 0.5f;
    ProjectedRefrTexCoords.y = -pin.RefractionMapSamplingPos.y / pin.RefractionMapSamplingPos.w / 2.0f + 0.5f;
    float2 perturbatedRefrTexCoords = ProjectedRefrTexCoords + perturbation;
    float4 refractiveColor = RefractionMap.Sample(gsamLinearWrap, perturbatedRefrTexCoords);

    float3 eyeVector = normalize(gEyePosW - pin.PosW.xyz);
    float3 normalVector = float3(0, 1, 0);

    
/////////////////////////////////////////////////
// FRESNEL TERM APPROXIMATION
/////////////////////////////////////////////////
    float fresnelTerm = (float) 0;

    if (gFresnelMode == 1)
    {
        fresnelTerm = 0.02 + 0.97f * pow((1 - dot(eyeVector, normalVector)), 5);
    }
    else if (gFresnelMode == 0)
    {
        fresnelTerm = 1 - dot(eyeVector, normalVector) * 1.3f;
    }
    else if (gFresnelMode == 2)
    {
        float fangle = 1.0f + dot(eyeVector, normalVector);
        fangle = pow(fangle, 5);
	   // fresnelTerm = fangle*50;
        fresnelTerm = 1 / fangle;
    }
    
    //    fresnelTerm = (1/pow((fresnelTerm+1.0f),5))+0.2f; // 
	    
	//Hardness factor - user input
    fresnelTerm = fresnelTerm * gDrawMode;
    
	//just to be sure that the value is between 0 and 1;
    fresnelTerm = fresnelTerm < 0 ? 0 : fresnelTerm;
    fresnelTerm = fresnelTerm > 1 ? 1 : fresnelTerm;
    
	// creating the combined color
    float4 combinedColor = refractiveColor * (1 - fresnelTerm) + reflectiveColor * fresnelTerm;

    
    /////////////////////////////////////////////////
	// WATER COLORING
	/////////////////////////////////////////////////

    float4 dullColor = float4(0.1f, 0.1f, 0.2f, 1.0f);
		//float4 dullColor = float4(0.0f, 1.0f, 1.4f, 1.0f);
	    
	   // float dullBlendFactor = 0.3f;
	   // float dullBlendFactor = 0.1f;
    float dullBlendFactor = gDullBlendFactor;

		// eredeti v?
		//Output.Color = dullBlendFactor*dullColor + (1-dullBlendFactor)*combinedColor;
	    
		// kicsit s??ebb a t??k?, mint az eredeti...
    pout.Albedo = (dullBlendFactor * dullColor + (1 - dullBlendFactor) * combinedColor);
	//Output.Color = (dullBlendFactor*dullColor + (1-dullBlendFactor)*combinedColor)*1.25f;
    
    
/////////////////////////////////////////////////
// Specular Highlights
/////////////////////////////////////////////////

    float4 speccolor;

    float3 lightSourceDir = normalize(float3(0.1f, 0.6f, 0.5f));

    float3 halfvec = normalize(eyeVector + lightSourceDir + float3(perturbation.x * gSpecPerturb, perturbation.y * gSpecPerturb, 0));
	
    float3 temp = 0;

    temp.x = pow(dot(halfvec, normalVector), gSpecPower);
	
    speccolor = float4(0.98, 0.97, 0.7, 0.6);
	
    speccolor = speccolor * temp.x;

    speccolor = float4(speccolor.x * speccolor.w, speccolor.y * speccolor.w, speccolor.z * speccolor.w, 0);

    pout.Albedo = pout.Albedo + speccolor;

    
    return pout;
}