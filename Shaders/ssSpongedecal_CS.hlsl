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

RWTexture2D<float4> OutputTexture : register(u0); // RWTexture2D로 출력 버퍼를 설정
RWTexture2D<float4> OrmOutputTexture : register(u1); // RWTexture2D로 출력 버퍼를 설정

sampler gsamLinearClamp : register(s0);

[numthreads(16, 16, 1)] // 각 워크그룹은 16x16 스레드를 사용
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    // 화면의 크기 내에서 현재 스레드의 위치를 구합니다.
    float2 uv = float2(DTid.xy) / float2(fTargetWidth, fTargetHeight);
    
    // 깊이 텍스처에서 월드 위치를 샘플링합니다.
    float3 vWorldPos = gDepthTexture.SampleLevel(gsamLinearClamp, uv, 0).xyz;

    // 월드 위치를 로컬 공간으로 변환합니다.
    float4 vLocalPos = mul(float4(vWorldPos, 1.f), gWorldInv);

    // 로컬 위치를 절대 값으로 변환하여 거리를 계산합니다.
    float3 dist = abs(vLocalPos.xyz);
    
    // 일정 거리 이상일 때 클리핑합니다.
    if (0.5f - max(dist.x, max(dist.y, dist.z)) < 0) 
        return;

    // 데칼 텍스처의 UV를 계산합니다.
    float2 decalUV = (vLocalPos.xz + 0.5f);

    // 알베도 텍스처를 샘플링합니다.
    float4 albedo = gAlbedo.SampleLevel(gsamLinearClamp, decalUV, 0);
    
    // 월드 공간에서 표면의 법선 벡터를 가져옵니다.
    float3 vNormal = gNormalTexture.SampleLevel(gsamLinearClamp, uv, 0).xyz * 2.0f - 1.0f;

    // 데칼 방향과 표면 법선 벡터의 내적을 계산합니다.
    float dotProduct = dot(normalize(vNormal), normalize(gDecalDirection));

    // 내적이 0보다 작으면 법선이 데칼 방향을 가리키지 않으므로 클리핑합니다.
    if (dotProduct < 0)
        return;


    // 페이드 팩터를 알베도에 곱해 출력합니다.
    //float4 finalColor = albedo * gFadeFactor;
    float4 finalColor = albedo;
    
    finalColor.a *= gFadeFactor;

    // 결과를 출력 텍스처에 저장합니다.
    // 알파 블렌딩을 잘 해와야한다.
    float4 prevColor = OutputTexture[DTid.xy];
    
    finalColor = lerp(prevColor, finalColor, finalColor.a);
    
    OutputTexture[DTid.xy] = finalColor;
    
    OrmOutputTexture[DTid.xy] = float4(0, 0.9f, 0.05, 0) * finalColor.a;

}
