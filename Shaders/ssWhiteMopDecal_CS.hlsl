cbuffer cbPerObject
{
    float4x4 gWVP;
};

cbuffer cbPerFrame
{
    float4x4 gViewInv;
    float4x4 gWorldInv;
    float fTargetWidth;
    float fTargetHeight;
};

cbuffer cbDecal
{
    float gFadeFactor;
    float3 gDecalDirection; 
};

Texture2D gAlbedo : register(t0);
Texture2D gNormalTexture : register(t1);
Texture2D gDepthTexture : register(t3);
// ����ũ �ý����� r���� ��   
Texture2D gMaskTexture : register(t4); // ����ũ �ؽ�ó �߰�
Texture2D gRoughnessTexture : register(t5);

RWTexture2D<float4> OutputTexture : register(u0); // RWTexture2D�� ��� ���۸� ����
RWTexture2D<float4> OrmOutputTexture : register(u1); // RWTexture2D�� ��� ���۸� ����

sampler gsamLinearClamp : register(s0);

[numthreads(16, 16, 1)] // �� ��ũ�׷��� 16x16 �����带 ���
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    // ȭ���� ũ�� ������ ���� �������� ��ġ�� ���մϴ�.
    float4 baseColor = { 0.385417, 0.31468, 0.250922, 1.f };
    float roughnessMultiplier = 0.5f;
    
    float2 uv = float2(DTid.xy) / float2(fTargetWidth, fTargetHeight);
    
    // ���� �ؽ�ó���� ���� ��ġ�� ���ø��մϴ�.
    float3 vWorldPos = gDepthTexture.SampleLevel(gsamLinearClamp, uv, 0).xyz;

    // ���� ��ġ�� ���� �������� ��ȯ�մϴ�.
    float4 vLocalPos = mul(float4(vWorldPos, 1.f), gWorldInv);

    // ���� ��ġ�� ���� ������ ��ȯ�Ͽ� �Ÿ��� ����մϴ�.
    float3 dist = abs(vLocalPos.xyz);
    
    // ���� �Ÿ� �̻��� �� Ŭ�����մϴ�.
    if (0.5f - max(dist.x, max(dist.y, dist.z)) < 0) 
        return;

    // ��Į �ؽ�ó�� UV�� ����մϴ�.
    float2 decalUV = (vLocalPos.xz + 0.5f);

    // �˺��� �ؽ�ó�� ���ø��մϴ�.
    float4 albedo = gAlbedo.SampleLevel(gsamLinearClamp, decalUV, 0);

    // ���� �������� ǥ���� ���� ���͸� �����ɴϴ�.
    float3 vNormal = gNormalTexture.SampleLevel(gsamLinearClamp, uv, 0).xyz * 2.0f - 1.0f;

    // ��Į ����� ǥ�� ���� ������ ������ ����մϴ�.
    float dotProduct = dot(normalize(vNormal), normalize(gDecalDirection));

    // ������ 0���� ������ ������ ��Į ������ ����Ű�� �����Ƿ� Ŭ�����մϴ�.
    if (dotProduct < 0)
        return;
    
    // ���̵� ���͸� �˺����� ���� ����մϴ�.
    //float4 finalColor = albedo * gFadeFactor;
    float4 finalColor = albedo;
    
    finalColor.a *= gFadeFactor;
    
    // ����ũ �ؽ�ó�� ���ø��մϴ�.
    float mask = gMaskTexture.SampleLevel(gsamLinearClamp, decalUV, 0).r;
    
    float roughness = gRoughnessTexture.SampleLevel(gsamLinearClamp, decalUV, 0).r;
    
    roughness *= roughnessMultiplier;
  
    // ����ũ ���� 0�̸� ������� �ʽ��ϴ�.
    if (mask == 0)
        return;
    
    finalColor.a *= mask;
    
    finalColor *= baseColor;

    // ����� ��� �ؽ�ó�� �����մϴ�.
    // ���� ������ �� �ؿ;��Ѵ�.
    float4 prevColor = OutputTexture[DTid.xy];
    
    finalColor = lerp(prevColor, finalColor, finalColor.a);
    
    OutputTexture[DTid.xy] = finalColor;
    
    // ORM �ؽ�ó�� ����մϴ�.  
    float4 orm = float4(0, roughness, -1, 0);
    
    OrmOutputTexture[DTid.xy] = orm;
}
