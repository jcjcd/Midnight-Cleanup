

cbuffer cbPerObject
{
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
    float4x4 gTexTransform;
    float4x4 gView;
    float4x4 gProj;
    float4x4 gViewProj;
};

cbuffer cbPerCamera
{
    float3 gEyePosW;
    float gPadding;
};

cbuffer cbEmissive
{
    float gEmissiveFactor;
};

// light

#define MAX_LIGHTS 3

struct DirectionalLight
{
    float3 color;
    float intensity;
    float3 direction;
    int isOn;
    float4 cascadedEndClip;
    matrix viewProj[4];
    int useShadow;
    int lightMode;
    float2 padding;
};

struct PointLight
{
    float3 color;
    float intensity;
    float3 position;
    float range;
    float3 attenuation;
    int isOn;
    matrix viewProj[6];
    int useShadow;
    float nearZ;
    int lightMode;
    float padding;
};

struct SpotLight
{
    float3 color;
    float intensity;
    float3 position;
    float range;
    float3 direction;
    float spot;
    float3 attenuation;
    int isOn;
    matrix viewProj;
    float spotAngle;
    int useShadow;
    int lightMode;
    float inner;
};