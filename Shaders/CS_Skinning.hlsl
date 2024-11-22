// Global variable access via cb
cbuffer BoneMatrices : register(b0)
{
    float4x4 gBoneTransforms[96];
};

cbuffer VertexCount : register(b1)
{
    uint gVertexCount;
}

struct BasicVertex
{
    float3 position;
    float3 normal;
    float3 tangent;
    float2 uv;
    float3 bitangent;
};

struct BoneWeight
{
    int4 boneIndices;
    float4 weights;
};

StructuredBuffer<BasicVertex> gVertices : register(t0);
StructuredBuffer<BoneWeight> gBoneWeights : register(t1);
RWStructuredBuffer<BasicVertex> gOutputVertices : register(u0);


[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= gVertexCount)
        return;

    float4x4 worldMat;
    
    worldMat = mul(gBoneTransforms[gBoneWeights[dispatchThreadID.x].boneIndices.x], gBoneWeights[dispatchThreadID.x].weights.x);
    worldMat += mul(gBoneTransforms[gBoneWeights[dispatchThreadID.x].boneIndices.y], gBoneWeights[dispatchThreadID.x].weights.y);
    worldMat += mul(gBoneTransforms[gBoneWeights[dispatchThreadID.x].boneIndices.z], gBoneWeights[dispatchThreadID.x].weights.z);
    worldMat += mul(gBoneTransforms[gBoneWeights[dispatchThreadID.x].boneIndices.w], gBoneWeights[dispatchThreadID.x].weights.w);
    
    BasicVertex vertex = gVertices[dispatchThreadID.x];
    float4 newPos = mul(float4(vertex.position, 1.0f), worldMat);
    float4 newNormal = mul(float4(vertex.normal, 0.0f), worldMat);
    float4 newTangent = mul(float4(vertex.tangent, 0.0f), worldMat);
    float4 newBitangent = mul(float4(vertex.bitangent, 0.0f), worldMat);
    
    gOutputVertices[dispatchThreadID.x].position = newPos.xyz;
    gOutputVertices[dispatchThreadID.x].normal = newNormal.xyz;
    gOutputVertices[dispatchThreadID.x].tangent = newTangent.xyz;
    gOutputVertices[dispatchThreadID.x].bitangent = newBitangent.xyz;
    gOutputVertices[dispatchThreadID.x].uv = vertex.uv;
}