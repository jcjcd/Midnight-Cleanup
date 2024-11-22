#pragma once

#include "IModelParser.h"

class RendererContext;
class Material;
struct Bone;
struct EntityNode;

class AssimpParser : public IModelParser
{
public:
	// IModelParser��(��) ���� ��ӵ�
	bool LoadModel(Renderer* renderer, const std::string& fileName, Model* model, ModelParserFlags parserFlags) override;

	// ���� �ִϸ��̼� fbx �ϳ��� �ҷ����� �Ŷ�� �� �Լ��� �������
	bool LoadAnimation(const std::string& fileName, AnimationClip* animationClip, ModelParserFlags flag);

	bool LoadAnimations(const std::string& fileName, std::vector<AnimationClip*>& animations, ModelParserFlags flag);

	// ������ ����..
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

