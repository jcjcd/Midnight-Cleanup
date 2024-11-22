#pragma once

#include "RendererDLL.h"


class Buffer;
class Renderer;
class Mesh;

struct BoneWeight
{
	uint32_t boneIndex[4];
	float weight[4];
};

struct Bone
{
	std::string name;

	// ToRoot 결과값 바로 제공
	Matrix offsetMatrix;
};

struct Node
{
public:
	Node* parent;

	uint32_t childCount = 0;
	std::vector<Node*> children;

	Mesh* mesh;
	uint32_t subMeshCount = 0;

	// animation 계산을 위한 트랜스폼
	Matrix nodeTransform;

	// 처음부터 toroot 계산한 트랜스폼
	Matrix toRootTransform;

	std::string name;
	uint32_t index = 0;
	uint32_t parentIndex = 0;
};

struct ANIMAVISION_DLL EntityNode 
{
	EntityNode* parent;

	uint32_t childCount = 0;
	std::vector<EntityNode*> children;

	std::string name;

	Matrix NodeTransform;

	uint32_t meshCount = 0;

	bool isSkinned = false;
};


struct Keyframe
{
	float timePos = 0.0f;
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Quaternion rotation;
	Vector3 translation = { 0.0f, 0.0f, 0.0f };
};

struct NodeClip
{
	std::string nodeName;
	std::vector<Keyframe> keyframes;
};

struct AnimationClip
{
	std::string name;
	float duration = 0.0f;
	float framePerSecond = 0.0f;
	uint32_t frameCount = 0;
	std::vector<std::shared_ptr<NodeClip>> nodeClips;

	// 밑에꺼 왜필요한지 모르는데 일단 저장은 해둘까...?
	bool isLoop = false;
	bool isPlay = false;
	float currentTimePos = 0.0f;
};

// TODO 이거 IMesh느낌으로 가야겠다..
// UV 3개나 4개나 막 늘어날때에 대해 확장을 해주어야하나..

struct SubMeshDescriptor
{
	SubMeshDescriptor() = default;
	SubMeshDescriptor(const std::string& name, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexCount, uint32_t vertexOffset)
		: name(name), indexCount(indexCount), indexOffset(indexOffset), vertexCount(vertexCount), vertexOffset(vertexOffset)
	{ }

	std::string name;
	uint32_t indexCount = 0;
	uint32_t indexOffset = 0;
	uint32_t vertexCount = 0;
	uint32_t vertexOffset = 0;

	// <이름, <인덱스, 오프셋매트릭스>>
	// 메쉬마다 인덱스가 달라질 수 있기 때문에 여기에 들어와야 한다..
	std::unordered_map<std::string, std::pair<uint32_t, Matrix>> boneIndexMap;

	// 창 : 바운드박스 추가하긴 해야된다. 피킹을 위해
	// BoundingBox boundingBox;
};

class ANIMAVISION_DLL Mesh
{
public:
	std::string name;

	std::vector<Vector3> vertices;
	std::vector<uint32_t> indices;
	std::vector<Vector3> normals;
	std::vector<Vector2> uv;
	std::vector<Vector2> uv2;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;
	std::vector<BoneWeight> boneWeights;
	std::vector<Vector4> colors;

	std::shared_ptr<Buffer> vertexBuffer = nullptr;
	std::shared_ptr<Buffer> indexBuffer = nullptr;

	uint32_t subMeshCount = 0;
	std::vector<SubMeshDescriptor> subMeshDescriptors;

	// 창 : 여기도 바운딩박스 추가해야한다.
	DirectX::BoundingBox boundingBox;
	DirectX::BoundingSphere boundingSphere;

	void CreateBuffers(Renderer* renderer);

	uint32_t GetVertexCount() const
	{
		return static_cast<uint32_t>(vertices.size());
	}

	uint32_t GetIndexCount() const
	{
		return static_cast<uint32_t>(indices.size());
	}
};

namespace cereal
{
	template <class Archive>
	void serialize(Archive& archive, DirectX::BoundingBox& bb)
	{
		archive(bb.Center, bb.Extents);
	}

	template <class Archive>
	void serialize(Archive& archive, DirectX::BoundingSphere& bs)
	{
		archive(bs.Center, bs.Radius);
	}

	template <class Archive>
	void serialize(Archive& archive, DirectX::XMFLOAT3& flt3)
	{
		archive(flt3.x, flt3.y, flt3.z);
	}

	template <class Archive>
	void serialize(Archive& archive, BoneWeight& bw)
	{
		archive(bw.boneIndex, bw.weight);
	}

	template <class Archive>
	void serialize(Archive& archive, SubMeshDescriptor& smd)
	{
		archive(
			smd.name, 
			smd.indexCount, 
			smd.indexOffset, 
			smd.vertexCount, 
			smd.vertexOffset,
			smd.boneIndexMap
		);
	}

	template <class Archive>
	void serialize(Archive& archive, Vector2& v2)
	{
		archive(v2.x, v2.y);
	}

	template <class Archive>
	void serialize(Archive& archive, Vector4& v4)
	{
		archive(v4.x, v4.y, v4.z, v4.w);
	}

	template <class Archive>
	void serialize(Archive& archive, Matrix& mat)
	{
		archive(mat._11, mat._12, mat._13, mat._14,
			mat._21, mat._22, mat._23, mat._24,
			mat._31, mat._32, mat._33, mat._34,
			mat._41, mat._42, mat._43, mat._44);
	}

	template <class Archive>
	void serialize(Archive& archive, Mesh& mesh)
	{
		archive(mesh.name,
		        mesh.vertices, mesh.indices, mesh.normals, mesh.uv, mesh.uv2, mesh.tangents, mesh.bitangents,
		        mesh.boneWeights, mesh.colors,
		        mesh.subMeshCount, mesh.subMeshDescriptors,
		        mesh.boundingBox, mesh.boundingSphere);

	}
	
	template <class Archive>
	void serialize(Archive& archive, AnimationClip& animationClip)
	{
		archive(animationClip.name,
			animationClip.duration, animationClip.framePerSecond, animationClip.frameCount, animationClip.nodeClips, 
			animationClip.isLoop, animationClip.isPlay, animationClip.currentTimePos);
	}

	template <class Archive>
	void serialize(Archive& archive, NodeClip& nodeClip)
	{
		archive(nodeClip.nodeName, nodeClip.keyframes);
	}

	template <class Archive>
	void serialize(Archive& archive, Keyframe& keyframe)
	{
		archive(keyframe.timePos, keyframe.scale, keyframe.rotation, keyframe.translation);
	}

	template <class Archive>
	void serialize(Archive& archive, Quaternion& quaternion)
	{
		archive(quaternion.x, quaternion.y, quaternion.z, quaternion.w);
	}

}

class ANIMAVISION_DLL MeshGenerator
{
public:
	static std::shared_ptr<Mesh> CreateCube(float width, float height, float depth);
	static std::shared_ptr<Mesh> CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount);
	static std::shared_ptr<Mesh> CreateQuad(Vector2 leftBottom, Vector2 rightTop, float z);
	// 캡슐이랑 이런거 나중에 추가해주지뭐
	static std::shared_ptr<Mesh> CreateTriangle();
};