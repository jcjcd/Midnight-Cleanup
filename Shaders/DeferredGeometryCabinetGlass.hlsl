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
    float2 SubTexC : TEXCOORD1;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD0;
    float2 SubTexC : TEXCOORD1;
    float ClipSpacePosZ : TEXCOORD2;
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

cbuffer cbMaskBuffer
{
    int gRecieveDecal;
    int gReflect;
    int gRefract;
};

Texture2D gGlassMask;
Texture2D gMicroDetail;
Texture2D gAlbedo;
TextureCube gReflectionCube;
//Texture2D gEmissive;

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Transform to world space
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // ��յ� ��� Ȯ�밡 �������� �ֱ� ������ ������ ����ġ����� ����Ѵ�..
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorldInvTranspose);
    vout.TangentW = mul(vin.TangentL, (float3x3) gWorld);
    
    // �븻�� ����ȭ�Ѵ�.
    vout.NormalW = normalize(vout.NormalW);
    vout.TangentW = normalize(vout.TangentW);

    // Transform to homogeneous clip space
    vout.PosH = mul(posW, gViewProj);

    // Output texture coordinates
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform).xy;
    vout.SubTexC = vin.SubTexC;

    // Clip space z
    vout.ClipSpacePosZ = vout.PosH.z;
    
    return vout;
}

PixelOut PSMain(VertexOut pin, bool isFrontFace : SV_IsFrontFace)  
{
    float4 PreshaderBuffer[16];
    PreshaderBuffer[0] = float4(0.000000, 0.000000, 0.000000, 0.000000); //(Unknown)
    PreshaderBuffer[1] = float4(-0.123810, -0.200000, 1.000000, 1.000000); //(Unknown)
    PreshaderBuffer[2] = float4(-0.123810, -0.200000, 1.000000, 0.183153); //(Unknown)
    PreshaderBuffer[3] = float4(2.430782, 0.738893, 4.000000, 3.000000); //(Unknown)
    PreshaderBuffer[4] = float4(3.000000, 1.000000, 0.500000, 0.500000); //(Unknown)
    PreshaderBuffer[5] = float4(0.000000, 0.100000, 0.000000, 0.000000); //(Unknown)
    PreshaderBuffer[6] = float4(0.000000, 0.000000, 0.000000, 0.000000); //(Unknown)
    PreshaderBuffer[7] = float4(0.536458, 0.488430, 0.420177, 1.000000); //(Unknown)
    PreshaderBuffer[8] = float4(3.040100, 3.077364, 2.029330, 0.000000); //(Unknown)
    PreshaderBuffer[9] = float4(1.000000, 1.000000, 1.000000, 0.000000); //(Unknown)
    PreshaderBuffer[10] = float4(1.000000, 1.000000, 1.000000, 0.200000); //(Unknown)
    PreshaderBuffer[11] = float4(0.200000, 0.536458, 0.488430, 0.420177); //(Unknown)
    PreshaderBuffer[12] = float4(0.250000, 0.198732, 0.073070, 1.000000); //(Unknown)
    PreshaderBuffer[13] = float4(0.250000, 0.198732, 0.073070, 1.000000); //(Unknown)
    PreshaderBuffer[14] = float4(0.980177, 1.000000, 0.547619, 0.747619); //(Unknown)
    PreshaderBuffer[15] = float4(0.000000, 0.000000, 0.000000, 0.000000); //(Unknown)
    
    PixelOut pout = (PixelOut) 0.0f;
    
    
// �ʱ� ��� (��� ��꿡 �ʿ�)
    float2 Local0 = pin.SubTexC.xy;
    float4 Local2 = gGlassMask.Sample(gsamAnisotropicWrap, Local0); // gGlassMask �ؽ�ó ���ø�

    // �ؽ�ó ��ǥ�� �������� ����
    float scaleFactor1 = 1.0; // ���⼭ ������ ���� ������ ������ �߰��� �� �ֽ��ϴ�.
    float Local4 = Local2.g * PreshaderBuffer[2].w;
    float2 Local5 = pin.TexC.xy;
    float2 Local6 = Local5 * PreshaderBuffer[3].x; // �ؽ�ó ��ǥ �����ϸ� ���

    float4 Local8 = gGlassMask.Sample(gsamAnisotropicWrap, Local6); // gGlassMask ���ø�
    float Local10 = Local4 + Local8.r;
    float Local11 = Local10 * PreshaderBuffer[3].y;

    // ��� ���
    float3 Local12 = lerp(PreshaderBuffer[2].xyz, float3(0.0, 0.0, 1.0), Local11);
    pout.Normal = float4(Local12, 1.f);

    // gMicroDetail �ؽ�ó ���ø� �߰�
    float2 Local34 = Local5 * PreshaderBuffer[8].x; // TexCoord�� ������ ����
    float4 Local36 = gMicroDetail.Sample(gsamAnisotropicWrap, Local34); // gMicroDetail ���ø�
    float Local38 = Local36.g * PreshaderBuffer[8].y;
    float Local39 = PositiveClampedPow(Local38, PreshaderBuffer[8].z);
    float Local40 = saturate(Local39);
    float Local41 = 1.0 - Local40;

    // gGlassMask�κ��� ��Ż�� ���
    float2 Local48 = Local5 * PreshaderBuffer[13].w; // ������ ����
    float4 Local50 = gGlassMask.Sample(gsamAnisotropicWrap, Local48); // gGlassMask ���ø�
    float Local52 = Local50.a * PreshaderBuffer[14].x;
    float Local53 = lerp(Local52, 0.0, Local11);

    // MaterialNormal�� ���� ���� �Ǵ� ź��Ʈ ������ �� �ֽ��ϴ�.
    float3 MaterialNormal = pout.Normal.xyz;

    MaterialNormal = normalize(MaterialNormal);

    float3 CameraVector = gEyePosW - pin.PosW;
    CameraVector = normalize(CameraVector);
    
    float3 ReflectionVector = reflect(CameraVector, MaterialNormal);
    
    
        // gReflectionCube �ؽ�ó ���ø� �߰�
    float4 Local22 = gReflectionCube.Sample(gsamAnisotropicWrap, ReflectionVector); // gReflectionCube ���ø�
    float4 Local24 = gReflectionCube.SampleBias(gsamAnisotropicWrap, ReflectionVector, PreshaderBuffer[3].w);
    float3 Local26 = lerp(Local22.rgb, Local24.rgb, PreshaderBuffer[4].y);
    float Local27 = dot(Local26, float3(0.3, 0.59, 0.11));
    float3 Local28 = lerp(Local26, float3(Local27, Local27, Local27), PreshaderBuffer[4].w);


    // ������ �Է°� ���
    float Local13 = dot(MaterialNormal, CameraVector); // dot(�븻, ī�޶����)
    float Local14 = max(0.0, Local13);
    float Local15 = 1.0 - Local14;
    float Local16 = abs(Local15);
    float Local18 = PositiveClampedPow(max(Local16, 0.0001), PreshaderBuffer[3].z);
    float Local20 = Local18 * (1.0 - 0.04) + 0.04;

    pout.Albedo.rgb = lerp(Local28 * PreshaderBuffer[11].xyz, PreshaderBuffer[13].xyz * Local10, Local11);
    pout.Albedo.a = lerp(0.0, 1.0, Local11);
    pout.ORM.r = 1.0f;
    pout.ORM.g = lerp(1.0, PreshaderBuffer[8].z, Local11);
    pout.ORM.b = Local53;

    pout.Emissive.xyz = Local28 * PreshaderBuffer[6].xyz;
    pout.Emissive.a = 1.0f;
    pout.PositionW = float4(pin.PosW, pin.ClipSpacePosZ);
    //pout.PositionW = (pout.PositionW )
    
    pout.Mask.r = gRecieveDecal;
    pout.Mask.g = gReflect;
    pout.Mask.b = gRefract;


    return pout;
}
