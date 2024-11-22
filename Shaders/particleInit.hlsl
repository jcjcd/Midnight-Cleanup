#include "particleCommon.hlsli"

RWStructuredBuffer<Particle> gParticleBuffer;

[numthreads(256, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{   
    gParticleBuffer[id.x] = (Particle) 0;
}