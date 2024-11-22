#include "particleCommon.hlsli"
#include "SamplerStates.hlsl"

cbuffer cbDeadList
{
    uint gNumDeadParticles;
    uint3 DeadListCount_pad;
};

Texture2D gRandomTexture;
RWStructuredBuffer<Particle> gParticleBuffer;
ConsumeStructuredBuffer<uint> gDeadListToAllocFrom;

[numthreads(1024,1,1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if(id.x < gNumDeadParticles && id.x < gParticleInstance.NumToEmit)
    {
        Particle pa = (Particle) 0;   

        // 난수 생성
        float2 uv = float2(id.x / 1024.0, gParticleInstance.TimePos + gParticleInstance.RandomSeed);
        float4 randomValues0 = gRandomTexture.SampleLevel(gsamLinearWrap, uv, 0);
        float2 uv1 = float2((id.x + 1) / 1024.0, gParticleInstance.TimePos + gParticleInstance.RandomSeed);
        float4 randomValues1 = gRandomTexture.SampleLevel(gsamLinearWrap, uv1, 0);
        float2 uv2 = float2((id.x + 1) / 1024.0, gParticleInstance.TimePos + gParticleInstance.RandomSeed);
        float4 randomValues2 = gRandomTexture.SampleLevel(gsamLinearWrap, uv2, 0);

		// 파티클 데이터 초기화
        processShape(randomValues0, pa.PositionW, pa.Velocity);
        pa.Size = lerp(gParticleMain.StartSize[0], gParticleMain.StartSize[1], saturate((randomValues1.x + 1) * 0.5f));
        pa.Rotation = lerp(gParticleMain.StartRotation[0], gParticleMain.StartRotation[1], saturate((randomValues1.y + 1) * 0.5f));
        pa.Color = lerp(gParticleMain.StartColor[0], gParticleMain.StartColor[1], saturate((randomValues1.z + 1) * 0.5f));
        pa.StartColor = pa.Color;
        pa.LifeTime = lerp(gParticleMain.StartLifeTime[0], gParticleMain.StartLifeTime[1], saturate((randomValues1.w + 1) * 0.5f));
        pa.Age = 0.f;
        pa.GravityModifier = lerp(gParticleMain.GravityModifier[0], gParticleMain.GravityModifier[1], saturate((randomValues2.x + 1) * 0.5f));
        pa.EmitterPosition = float3(gParticleInstance.Transform[3][0], gParticleInstance.Transform[3][1], gParticleInstance.Transform[3][2]);

		// 죽은 파티클 인덱스를 가져와서 버퍼에 갱신
        uint index = gDeadListToAllocFrom.Consume();
        gParticleBuffer[index] = pa;
    }
}