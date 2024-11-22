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

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

Texture2D gReveal : register(t0);
Texture2D gAccum : register(t1);

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    vout.PosH = float4(vin.PosL, 1.0f);
	
    // Output texture coordinates
    vout.TexC = vin.TexC;

    return vout;
}

// Epsilon number
static const float EPSILON = 0.00001f;

// Calculate floating point numbers equality accurately
bool isApproximatelyEqual(float a, float b)
{
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}

// Get the max value between three values
float max3(float3 v)
{
    return max(max(v.x, v.y), v.z);
}


float4 PSMain(VertexOut pin) : SV_Target
{
    float4 output;

    // Fragment coordination
    int2 coords = pin.TexC;

    // Fragment revealage
    float revealage = gReveal.Sample(gsamLinearClamp, pin.TexC).r;

    // Save the blending and color texture fetch cost if there is not a transparent fragment
    if (isApproximatelyEqual(revealage, 1.0f))
        discard;

    // Fragment color
    float4 accumulation = gAccum.Sample(gsamLinearClamp, pin.TexC);

    // Suppress overflow
    if (isinf(max3(abs(accumulation.rgb))))
        accumulation.rgb = tofloat3(accumulation.a);

    // Prevent floating point precision bug
    float3 average_color = accumulation.rgb / max(accumulation.a, EPSILON);

    // Blend pixels
    output = float4(average_color, 1.0f - revealage);

    return output;
}
