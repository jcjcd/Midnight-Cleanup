#pragma once

#include "IModelParser.h"

class RendererContext;
class Material;
struct Bone;
struct EntityNode;

class AssimpParser : public IModelParser
{
public:
	// IModelParser을(를) 통해 상속됨
	bool LoadModel(Renderer* renderer, const std::string& fileName, Model* model, ModelParserFlags parserFlags) override;

	// 만약 애니메이션 fbx 하나씩 불러오는 거라면 이 함수만 사용하자
	bool LoadAnimation(const std::string& fileName, AnimationClip* animationClip, ModelParserFlags flag);

	bool LoadAnimations(const std::string& fileName, std::vector<AnimationClip*>& animations, ModelParserFlags flag);

	// 툴에서 쓸거..
	bool LoadModelForEntity(const std::string& fileName, Model* model, ModelParserFlags parserFlags) override;
private:

	void processNode2(Renderer* renderer, void* transform, struct aiNode* node, const struct aiScene* scene, std::vector<Mesh*>& meshes);
	Mesh* processMesh2(Renderer* renderer, void* transform, const struct aiScene* scene, const aiNode* node);

	void processAnimations(const struct aiScene* scene, std::vector<AnimationClip*>& animations, const std::string& filePath);
	void processNode3(Renderer* renderer, void* transform, struct aiNode* node, const struct aiScene* scene, std::vector<Mesh*>& meshes, std::vector<Node*>& nodes, Node* parent);
	void processNode4(Renderer* renderer, void* transform, struct aiNode* node, const struct aiScene* scene, std::vector<Mesh*>& meshes, std::vector<Node*>& nodes, Node* parent, uint32_t index);
	void processBone(const struct aiScene* scene, std::vector<Bone*>& bones);

	void processNodeForEntity(void* transform, struct aiNode* node, const struct aiScene* scene, EntityNode* parent, EntityNode** root);
};

