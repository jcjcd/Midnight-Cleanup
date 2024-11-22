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

RWTexture2D<float4> OutputTexture : register(u0); // RWTexture2D�� ��� ���۸� ����
RWTexture2D<float4> OrmOutputTexture : register(u1); // RWTexture2D�� ��� ���۸� ����

sampler gsamLinearClamp : register(s0);

[numthreads(16, 16, 1)] // �� ��ũ�׷��� 16x16 �����带 ���
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    // ȭ���� ũ�� ������ ���� �������� ��ġ�� ���մϴ�.
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

    // ����� ��� �ؽ�ó�� �����մϴ�.
    // ���� ������ �� �ؿ;��Ѵ�.
    float4 prevColor = OutputTexture[DTid.xy];
    
    finalColor = lerp(prevColor, finalColor, finalColor.a);
    
    OutputTexture[DTid.xy] = finalColor;
    
    OrmOutputTexture[DTid.xy] = float4(0, 0.9f, 0.05, 0) * finalColor.a;

}
