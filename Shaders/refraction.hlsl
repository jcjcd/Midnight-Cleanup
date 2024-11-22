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
    float3 normal : NORMAL;
    float clip : SV_ClipDistance0;
};


cbuffer ClipPlaneBuffer
{
    float4 clipPlane;
};



////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
VertexOut RefractionVertexShader(VertexIn input)
{
    VertexOut output;
    

    // Change the position vector to be 4 units for proper matrix calculations.
    float4 position = float4(input.PosL, 1.0f);

    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(position, gWorld);
    output.position = mul(output.position, gView);
    output.position = mul(output.position, gProj);
    
    // Store the texture coordinates for the pixel shader.
    output.tex = input.TexC;
    
    // Calculate the normal vector against the world matrix only.
    output.normal = mul(input.NormalL, (float3x3) gWorld);
    
    // Normalize the normal vector.
    output.normal = normalize(output.normal);


    // Set the clipping plane.
    output.clip = dot(mul(position, gWorld), clipPlane);

    return output;
}

/////////////
// GLOBALS //
/////////////
Texture2D shaderTexture;

cbuffer LightBuffer
{
    float4 ambientColor;
    float4 diffuseColor;
    float3 lightDirection;
};

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 RefractionPixelShader(VertexOut input) : SV_TARGET
{
    float4 textureColor;
    float3 lightDir;
    float lightIntensity;
    float4 color;
    

    // Sample the texture pixel at this location.
    textureColor = shaderTexture.Sample(gsamLinearClamp, input.tex);
    
    // Set the default output color to the ambient light value for all pixels.
    color = ambientColor;

    // Invert the light direction for calculations.
    lightDir = -lightDirection;

    // Calculate the amount of light on this pixel.
    lightIntensity = saturate(dot(input.normal, lightDir));

    if (lightIntensity > 0.0f)
    {
        // Determine the final diffuse color based on the diffuse color and the amount of light intensity.
        color += (diffuseColor * lightIntensity);
    }

    // Saturate the final light color.
    color = saturate(color);

    // Multiply the texture pixel and the input color to get the final result.
    color = color * textureColor;
    
    return color;
}
