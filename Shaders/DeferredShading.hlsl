#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"
#include "PBRSupport.hlsl"
#include "commonMath.hlsl"
#include "LightandShadowFunctions.hlsl"

cbuffer cbPerLight
{
   //DirectionalLight directionalLights[MAX_LIGHTS];
    DirectionalLight directionalLights;
    PointLight pointLights[MAX_LIGHTS];
    SpotLight spotLights[MAX_LIGHTS];
    PointLight nonShadowPointLights[3];

   //unsigned int numDirectionalLights;
    unsigned int numPointLights;
    unsigned int numSpotLights;
};

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

Texture2D gAlbedo;
Texture2D gORM;
Texture2D gNormal;
Texture2D gEmissive;
Texture2D gPositionW;
Texture2D gDecalOutputAlbedo;
Texture2D gDecalOutputORM;
Texture2D gPreCalculatedLighting;
Texture2D gMask;

Texture2DArray gDirectionLightShadowMap;
Texture2DArray gSpotLightShadowMap;
Texture2DArray gPointLightShadowMap;

TextureCube gIrradiance;
TextureCube gPrefilteredEnvMap;
Texture2D gBRDFLUT;


#define EMISSIVE_FACTOR 1.0f

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
    float4 albedoOrigin = gAlbedo.Sample(gsamPointClamp, pin.TexC);
    float4 decalAlbedo = gDecalOutputAlbedo.Sample(gsamPointClamp, pin.TexC);
    float4 decalORM = gDecalOutputORM.Sample(gsamPointClamp, pin.TexC);
    
    float recieveDecal = gMask.Sample(gsamPointClamp, pin.TexC).r;

    float3 albedo = albedoOrigin.rgb;
    // 일단 대충 럴프한다.
    //float3 albedo = lerp(albedoOrigin.rgb, decalAlbedo.rgb, decalAlbedo.a);
    if (recieveDecal > 0.5)
    {
        albedo = lerp(albedoOrigin.rgb, decalAlbedo.rgb, decalAlbedo.a);
    }
    
    float3 normal = gNormal.Sample(gsamPointClamp, pin.TexC).xyz;
    normal = normalize(normal);
    float4 emissive = gEmissive.Sample(gsamPointClamp, pin.TexC);
    float4 precalculatedLightData = gPreCalculatedLighting.Sample(gsamPointClamp, pin.TexC);
    float3 pcl = precalculatedLightData.xyz;
    float emissiveFactor = gORM.Sample(gsamPointClamp, pin.TexC).w;

    if (normal.x > 100.0f)
    {
        return float4(albedo, 1.0f) + emissive * emissiveFactor;
        //return float4(albedo, 1.0f);
    }

    normal = normalize(normal);
    float metallic = gORM.Sample(gsamPointClamp, pin.TexC).z;
    float roughness = gORM.Sample(gsamPointClamp, pin.TexC).y;

    if (recieveDecal > 0.5)
    {
        metallic = decalORM.b <= 0.0f ? metallic : decalORM.b;
        roughness = decalORM.g <= 0.0f ? roughness : decalORM.g;
    }

    float ao = gORM.Sample(gsamPointClamp, pin.TexC).x;
    float4 position = gPositionW.Sample(gsamPointClamp, pin.TexC);
    float3 positionW = position.xyz;
    float clipSpacePosZ = position.w;

    float3 V = normalize(gEyePosW - positionW);
    V = normalize(V);
    float ndotv = saturate(dot(normal, V));
    
    float3 F0 = tofloat3(0.04f);
    F0 = lerp(F0, albedo, metallic);

    float3 directLighting = tofloat3(0.0f);

    bool isBakedArea = false;
    if (precalculatedLightData.a < -100.0f)
    {
        isBakedArea = true;
        pcl = 0.0f;
    }

    //for(uint dIndex=0; dIndex<numDirectionalLights;dIndex++)
    {
        float3 currentDirectLighting = tofloat3(0.0f);
        
        //if(directionalLights[dIndex].isOn==0)
        if (directionalLights.isOn == 1)
        {
                 //directLighting += currentDirectLighting;
                 //continue;

             
             // Specular coefficiant - fixed reflectance value for non-metals
             //float3 L = -directionalLights[dIndex].direction;
            float3 L = -directionalLights.direction;
            L = normalize(L);

             // Half vector
            float3 H = normalize(L + V);

             // products
            float ndotl = saturate(dot(normal, L));

             // Specular
            float3 F = fresnelSchlick(max(0.0, dot(H, V)), F0); // 프레넬
            float D = DistributionGGXPow32(normal, H, roughness); // 정반사 분포도
            float G = GeometrySmith(normal, V, L, roughness); // 기하 감쇠율

            float3 specular = F * G * D / max(0.00001f, ndotl * ndotv);
            
             // Diffuse
            float3 kD = lerp(tofloat3(1.0f) - F, tofloat3(0.0f), metallic);
            float3 diffuse = (kD * albedo) / PI;

             //currentDirectLighting = (diffuse + specular) * ndotl * directionalLights[dIndex].color * directionalLights[dIndex].intensity;
            currentDirectLighting = (diffuse + specular) * ndotl * directionalLights.color * directionalLights.intensity;

            currentDirectLighting = clamp(currentDirectLighting, 0.0f, 1.0f);

             // Shadow factor
             //if(directionalLights[dIndex].useShadow == 1)
            if (directionalLights.useShadow == 1)
            {
                float shadowFactor = 0.0f;
                 //float cascades[3] = {directionalLights[dIndex].cascadedEndClip.x, directionalLights[dIndex].cascadedEndClip.y, directionalLights[dIndex].cascadedEndClip.z};
                float cascades[4] = { directionalLights.cascadedEndClip.x, directionalLights.cascadedEndClip.y, directionalLights.cascadedEndClip.z, directionalLights.cascadedEndClip.w };

                 //for(uint cascadeIdx = 0; cascadeIdx < 3; ++cascadeIdx)
                for (uint cascadeIdx = 0; cascadeIdx < 4; ++cascadeIdx)
                {
                    float curEndClip;
                    if (cascadeIdx == 0)
                        curEndClip = cascades[0];
                    else if (cascadeIdx == 1)
                        curEndClip = cascades[1];
                     //else
                    else if (cascadeIdx == 2)
                        curEndClip = cascades[2];
                    else
                        curEndClip = cascades[3];

                    if (clipSpacePosZ > curEndClip)            
                        continue;

                     //float4 shadowPos = mul(float4(positionW, 1.0f), directionalLights[dIndex].viewProj[cascadeIdx]);
                    float4 shadowPos = mul(float4(positionW, 1.0f), directionalLights.viewProj[cascadeIdx]);
                    shadowPos.x = shadowPos.x * 0.5f + 0.5f;
                    shadowPos.y = -shadowPos.y * 0.5f + 0.5f;
                     //shadowFactor = CalculateCascadeShadowRatio(gsamShadow, gDirectionLightShadowMap, shadowPos, dIndex*3+cascadeIdx, 2048.0f);
                    shadowFactor = CalculateCascadeShadowRatio(gsamShadow, gDirectionLightShadowMap, shadowPos, cascadeIdx, 2048.0f);

                     //switch  (directionalLights[dIndex].lightMode)
                    switch (directionalLights.lightMode)
                    {
                         // real-time light
                        case 0:
                            currentDirectLighting *= shadowFactor;
                            break;
                         // mixed light
                        case 1:
                         {
                                if (isBakedArea)
                                    currentDirectLighting *= shadowFactor;
                                else
                                {
                                    float3 realtimeShadow = pcl - currentDirectLighting * (1 - shadowFactor);
                                 
                                 // 그림자 색상의 최솟값을 두어 부드럽게 표현
                                    realtimeShadow = max(realtimeShadow, 0.001f);

                                 // 그림자 세기로 보간, 현재는 상수로 적용
                                    realtimeShadow = lerp(pcl, realtimeShadow, 1);

                                    pcl = min(pcl, realtimeShadow);
                                }
                                break;
                            }
                    }
                    break;
                }
            }
        }


        directLighting += currentDirectLighting;
    }

    for (uint pIndex = 0; pIndex < numPointLights; ++pIndex)
    {
        float3 resultColor = tofloat3(0.0f);
        if (pointLights[pIndex].isOn == 0)
        {
            directLighting += resultColor;
            continue;
        }
        
        // Specular coefficiant - fixed reflectance value for non-metals
        float3 lightVector = pointLights[pIndex].position - positionW;
        float distance = length(lightVector);
        
        if (distance > pointLights[pIndex].range)
            continue;
        
        lightVector = normalize(lightVector);
        
        // Half vector
        float3 H = normalize(V + lightVector);
        
        // products
        float NdotV = saturate(dot(normal, V));
        float NdotL = saturate(dot(normal, lightVector));
        
        // Specular
        float3 F = fresnelSchlick(max(0.0, dot(H, V)), F0); // 프레넬
        float D = DistributionGGXPow(normal, H, roughness, 1.5); // 정반사 분포도
        float G = GeometrySmith(normal, V, lightVector, roughness); // 기하 감쇠율
        
        float3 specular = (F * D * G) / max(0.00001, 4.0 * NdotV * NdotL);
        
        // Diffuse
        float3 kD = lerp(tofloat3(1.0f) - F, tofloat3(0.0f), metallic);
        float3 diffuse = (kD * albedo) / PI;
        resultColor = (diffuse + specular) * NdotL * pointLights[pIndex].color * pointLights[pIndex].intensity;
        
        float att = 1.0f / dot(pointLights[pIndex].attenuation, float3(1.0f, distance, distance * distance));
        float smoothAtt = smoothstep(pointLights[pIndex].range * 0.75, pointLights[pIndex].range, distance);
        att *= (1.0 - smoothAtt);
        resultColor *= att;

        resultColor = clamp(resultColor, 0.0f, 1.0f);

        // Shadow factor
        if (pointLights[pIndex].useShadow == 1)
        {
            float shadowFactor = 0.0f;
            float4 fragToLight = float4(positionW - pointLights[pIndex].position, 1.0f);
            float4 normalizedDirection = normalize(fragToLight);
            float4 absDirection = float4(abs(normalizedDirection.x), abs(normalizedDirection.y), abs(normalizedDirection.z), 1.0f);
            uint rtIndexRemainder = 0;

            if (absDirection.x > absDirection.y && absDirection.x > absDirection.z)
            {
                if (fragToLight.x < 0.0f)
                    rtIndexRemainder = 1;
                else
                    rtIndexRemainder = 0;
            }
            else if (absDirection.y > absDirection.x && absDirection.y > absDirection.z)
            {
                if (fragToLight.y < 0.0f)
                    rtIndexRemainder = 3;
                else
                    rtIndexRemainder = 2;
            }
            else
            {
                if (fragToLight.z < 0.0f)
                    rtIndexRemainder = 5;
                else
                    rtIndexRemainder = 4;
            }

            for (uint texIndex = pIndex * 6; texIndex < (pIndex + 1) * 6; ++texIndex)
            {
                if (texIndex % 6 == rtIndexRemainder)
                {
                    float4 shadowPosTex = mul(float4(positionW, 1.0f), pointLights[pIndex].viewProj[rtIndexRemainder]);
                    //float curFactor = CalcPointShadowFactor2(gsamShadow, gPointLightShadowMap, fragToLight, shadowPosTex, pointLights[pIndex].range, pointLights[pIndex].nearZ, texIndex, 1024.0f);
                    float curFactor = PLShadowCalculation(shadowPosTex, fragToLight.xyz, gsamLinearClamp, gPointLightShadowMap, texIndex, pointLights[pIndex].range);
                    shadowFactor = curFactor;
                    break;
                }
            }

            switch (pointLights[pIndex].lightMode)
            {
                    // real-time light
                case 0:
                    resultColor *= shadowFactor;
                    break;
                    // mixed light
                case 1:
                    {
                        if (isBakedArea)
                            resultColor *= shadowFactor;
                        else
                        {
                            float3 realtimeShadow = pcl - resultColor * (1 - shadowFactor);
                            
                            // 그림자 색상의 최솟값을 두어 부드럽게 표현
                            realtimeShadow = max(realtimeShadow, 0.001f);

                            // 그림자 세기로 보간, 현재는 상수로 적용
                            realtimeShadow = lerp(pcl, realtimeShadow, 1);

                            pcl = min(pcl, realtimeShadow);
                        }
                        break;
                    }
            }
        }
        
        directLighting += resultColor;
    }

    for (uint pIndex = 0; pIndex < 3; pIndex++)
    {
	    float3 resultColor = tofloat3(0.0f);
		if (nonShadowPointLights[pIndex].isOn == 0)
        {
            directLighting += resultColor;
            continue;
        }

        float3 lightVector = nonShadowPointLights[pIndex].position - positionW;
        float distance = length(lightVector);
        
        if (distance > nonShadowPointLights[pIndex].range)
            continue;
        
        lightVector = normalize(lightVector);
        
        // Half vector
        float3 H = normalize(V + lightVector);
        
        // products
        float NdotV = saturate(dot(normal, V));
        float NdotL = saturate(dot(normal, lightVector));
        
        // Specular
        float3 F = fresnelSchlick(max(0.0, dot(H, V)), F0); // 프레넬
        float D = DistributionGGX(normal, H, roughness); // 정반사 분포도
        float G = GeometrySmith(normal, V, lightVector, roughness); // 기하 감쇠율
        
        float3 specular = (F * D * G) / max(0.00001, 4.0 * NdotV * NdotL);
        
        // Diffuse
        float3 kD = lerp(tofloat3(1.0f) - F, tofloat3(0.0f), metallic);
        float3 diffuse = (kD * albedo) / PI;
        resultColor = (diffuse + specular) * NdotL * nonShadowPointLights[pIndex].color * nonShadowPointLights[pIndex].intensity;
        
        float att = 1.0f / dot(nonShadowPointLights[pIndex].attenuation, float3(1.0f, distance, distance * distance));
        float smoothAtt = smoothstep(nonShadowPointLights[pIndex].range * 0.75, nonShadowPointLights[pIndex].range, distance);
        att *= (1.0 - smoothAtt);
        resultColor *= att;

        resultColor = clamp(resultColor, 0.0f, 1.0f);
        directLighting += resultColor;
    }

    for (uint sIndex = 0; sIndex < numSpotLights; sIndex++)
    {
        float3 resultColor = tofloat3(0.0f);
        if (spotLights[sIndex].isOn == 0)
        {
            directLighting += resultColor;
            continue;
        }
        
        // Specular coefficiant - fixed reflectance value for non-metals
        float3 lightVector = spotLights[sIndex].position - positionW;
        float distance = length(lightVector);
        
        if (distance > spotLights[sIndex].range)
            continue;
        
        lightVector = normalize(lightVector);

        float radian = spotLights[sIndex].spotAngle * (PI / 180.0f) * 0.5f;
        float radianInner = spotLights[sIndex].inner * (PI / 180.0f) * 0.5f;

        if (dot(-lightVector, spotLights[sIndex].direction) < cos(radian))
            continue;
        
        // Half vector
        float3 H = normalize(V + lightVector);
        
        // products
        float NdotV = saturate(dot(normal, V));
        float NdotL = saturate(dot(normal, lightVector));
        
        // Specular
        float3 F = fresnelSchlick(max(0.0, dot(H, V)), F0); // 프레넬
        float D = DistributionGGX(normal, H, roughness); // 정반사 분포도
        float G = GeometrySmith(normal, V, lightVector, roughness); // 기하 감쇠율
        
        float3 specular = (F * D * G) / max(0.00001, 4.0 * NdotV * NdotL);
        
        // Diffuse
        float3 kD = lerp(tofloat3(1.0f) - F, tofloat3(0.0f), metallic);
        float3 diffuse = (kD * albedo) / PI;
        
        resultColor = (diffuse + specular) * NdotL * spotLights[sIndex].color * spotLights[sIndex].intensity;
        
        float spot = dot(-lightVector, spotLights[sIndex].direction);
        spot = smoothstep(cos(radian), cos(radianInner), spot);
        float att = spot / dot(spotLights[sIndex].attenuation, float3(1.0f, distance, distance * distance));
        resultColor *= att;

        resultColor = clamp(resultColor, 0.0f, 1.0f);

        // Shadow factor
        if (spotLights[sIndex].useShadow == 1)
        {
            float4 shadowPos = mul(float4(positionW, 1.0f), spotLights[sIndex].viewProj);
            shadowPos /= shadowPos.w;
            float shadowFactor = 0;
            shadowFactor = CalculateShadowRatio(gsamShadow, gSpotLightShadowMap, shadowPos, sIndex, 1024.0f);

            switch (spotLights[sIndex].lightMode)
            {
                    // real-time light
                case 0:
                    resultColor *= shadowFactor;
                    break;
                    // mixed light
                case 1:
                    {
                        if (isBakedArea)
                            resultColor *= shadowFactor;
                        else
                        {
                            float3 realtimeShadow = pcl - resultColor * (1 - shadowFactor);
                            
                            // 그림자 색상의 최솟값을 두어 부드럽게 표현
                            realtimeShadow = max(realtimeShadow, 0.001f);

                            // 그림자 세기로 보간, 현재는 상수로 적용
                            realtimeShadow = lerp(pcl, realtimeShadow, 1);

                            pcl = min(pcl, realtimeShadow);
                        }
                        break;
                    }
            }
        }
        
        directLighting += resultColor;
    }
    
    
    float3 F = fresnelSchlickRoughness(max(dot(normal, V), 0.0), F0, roughness);
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    
    float3 irradiance = gIrradiance.Sample(gsamLinearClamp, normal).rgb;
    float3 diffuse = irradiance * albedo;

    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0f;
    float3 prefilteredColor = gPrefilteredEnvMap.SampleLevel(gsamLinearClamp, reflect(-V, normal), roughness * MAX_REFLECTION_LOD).rgb;
    float2 brdf = gBRDFLUT.Sample(gsamLinearClamp, float2(max(dot(normal, V), 0.0), roughness)).rg;
    float3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    // ambient lighting
    float ambientFactor = directionalLights.intensity * 0.3f;
    //float ambientFactor = directionalLights.intensity;
    float3 ambient = (kD * diffuse + specular) * ao * ambientFactor;
    float3 color = directLighting + ambient;
    //float3 color = directLighting;
    
    // HDR tonemapping
    //color = color / (color + tofloat3(1.0f));
    //color = pow(color, 1.0f / 2.2f);

    pcl *= albedo;

    float4 finalColor = float4(color + emissive.xyz * emissiveFactor + pcl, 1.0f);
    //float4 finalColor = float4(color + pcl, 1.0f);
    
    // 감마 보정
    //finalColor = pow(finalColor, 1.0f / 2.2f);
    
    
    // 포그
    //if(bUseFog == 1)
    //{
    //    float fogFactor = saturate((clipSpacePosZ - gFogStart) / gFogRange);
    //    finalColor = lerp(finalColor, float4(gFogColor, 1.0f), fogFactor);
    //}
    
    return finalColor;
    //return float4(color, 1.0f);

}


