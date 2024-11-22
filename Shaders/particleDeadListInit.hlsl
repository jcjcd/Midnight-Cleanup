#include "particleCommon.hlsli"

AppendStructuredBuffer<uint> gDeadListToAddTo;

[numthreads(256, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{   
    gDeadListToAddTo.Append(id.x);
}