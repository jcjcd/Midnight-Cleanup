#include "ShaderVariables.hlsl"
#include "SamplerStates.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 reflectionPosition : TEXCOORD1;
    float4 refractionPosition : TEXCOORD2;
};

cbuffer ReflectionBuffer
{
    matrix reflectionMatrix;
};

cbuffer WaterBuffer
{
    float waterTranslation;
    float reflectRefractScale;
    float2 padding;
};


Texture2D reflectionTexture;
Texture2D refractionTexture;
Texture2D normalTexture;


////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
VertexOut WaterVertexShader(VertexIn input)
{
    VertexOut output;
    matrix reflectProjectWorld;
    matrix viewProjectWorld;
    
    
    // Change the position vector to be 4 units for proper matrix calculations.
    float4 position = float4(input.PosL, 1.0f);

    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(position, gWorld);
    output.position = mul(output.position, gView);
    output.position = mul(output.position, gProj);
    
    // Store the texture coordinates for the pixel shader.
    output.tex = input.TexC;


    // Create the reflection projection world matrix.
    reflectProjectWorld = mul(reflectionMatrix, gProj);
    reflectProjectWorld = mul(gWorld, reflectProjectWorld);

    // Calculate the input position against the reflectProjectWorld matrix.
    output.reflectionPosition = mul(position, reflectProjectWorld);


    // Create the view projection world matrix for refraction.
    viewProjectWorld = mul(gView, gProj);
    viewProjectWorld = mul(gWorld, viewProjectWorld);
   
    // Calculate the input position against the viewProjectWorld matrix.
    output.refractionPosition = mul(position, viewProjectWorld);
    
    return output;
}


////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 WaterPixelShader(VertexOut input) : SV_TARGET
{
    float2 reflectTexCoord;
    float2 refractTexCoord;
    float4 normalMap;
    float3 normal;
    float4 reflectionColor;
    float4 refractionColor;
    float4 color;

    // Move the position the water normal is sampled from to simulate moving water. 
    input.tex.y += waterTranslation;

    // Calculate the projected reflection texture coordinates.
    reflectTexCoord.x = input.reflectionPosition.x / input.reflectionPosition.w / 2.0f + 0.5f;
    reflectTexCoord.y = -input.reflectionPosition.y / input.reflectionPosition.w / 2.0f + 0.5f;
    
    // Calculate the projected refraction texture coordinates.
    refractTexCoord.x = input.refractionPosition.x / input.refractionPosition.w / 2.0f + 0.5f;
    refractTexCoord.y = -input.refractionPosition.y / input.refractionPosition.w / 2.0f + 0.5f;



    // Sample the normal from the normal map texture.
    normalMap = normalTexture.Sample(gsamLinearClamp, input.tex);

    // Expand the range of the normal from (0,1) to (-1,+1).
    normal = (normalMap.xyz * 2.0f) - 1.0f;

    // Re-position the texture coordinate sampling position by the normal map value to simulate the rippling wave effect.
    reflectTexCoord = reflectTexCoord + (normal.xy * reflectRefractScale);
    refractTexCoord = refractTexCoord + (normal.xy * reflectRefractScale);

    // Sample the texture pixels from the textures using the updated texture coordinates.
    reflectionColor = reflectionTexture.Sample(gsamLinearClamp, reflectTexCoord);
    refractionColor = refractionTexture.Sample(gsamLinearClamp, refractTexCoord);

    // Combine the reflection and refraction results for the final color.
    color = lerp(reflectionColor, refractionColor, 0.6f);
    
    return color;
}
