#pragma once

#include "AssimpParser.h"

// 창: 테스트용 이거 나중에 다 인터페이스로 빼던지 해서 워닝 다 없애보자
#include "RendererDLL.h"

struct EntityNode;
class Mesh;
class RendererContext;
class Material;
struct Node;
struct AnimationClip;
struct Bone;

class ANIMAVISION_DLL Model
{
	friend class ModelLoader;

public:
	Model(const std::string& path);
	~Model();

	std::vector<Mesh*>& GetMeshes() { return m_Meshes; }
	std::vector<std::string>& GetMaterialNames() { return m_MaterialNames; }
	std::vector<Material*>& GetMaterials() { return m_Materials; }
	std::vector<AnimationClip*>& GetAnimations() { return m_Animations; }
	std::vector<Node*>& GetNodes() { return m_Nodes; }
	std::vector<Bone*>& GetBones() { return m_Bones; }

public:
	const std::string m_Path;

private:
	std::vector<Mesh*> m_Meshes;
	std::vector<std::string> m_MaterialNames;
	std::vector<Material*> m_Materials;
	std::vector<Node*> m_Nodes;
	std::vector<AnimationClip*> m_Animations;
	std::vector<Bone*> m_Bones;

	// 툴에서 쓸거
public:
	EntityNode* m_EntityRootNode = nullptr;
};

/// Model 생성과 제거 관리하는 클래스
class ANIMAVISION_DLL ModelLoader
{
public:
	ModelLoader() = delete;

	static Model* Create(Renderer* renderer, const std::string& filePath, ModelParserFlags parserFlags = ModelParserFlags::NONE);
	static Model* CreateForEntity(const std::string& filePath, ModelParserFlags parserFlags = ModelParserFlags::NONE);
	static void Reload(Renderer* renderer, Model& model, const std::string& filePath, ModelParserFlags parserFlags = ModelParserFlags::NONE);
	static bool Destroy(Model*& modelInstance);
	static AnimationClip* LoadAnimation(const std::string& filePath, ModelParserFlags parserFlags = ModelParserFlags::NONE);
	static void LoadAnimations(const std::string& filePath, std::vector<AnimationClip*>& animations, ModelParserFlags parserFlags /*= ModelParserFlags::NONE*/);

private:
	static AssimpParser m_AssimpParser;
};

