#include "pch.h"
#include "ModelLoader.h"
#include "Mesh.h"
#include "Material.h"

Model::Model(const std::string& path)
	: m_Path(path)
{

}

Model::~Model()
{
	for (auto& mesh : m_Meshes)
		if (mesh)
			delete mesh;

	for (auto& node : m_Nodes)
		if (node)
			delete node;

	for (auto& material : m_Materials)
		if (material)
			delete material;

	for (auto& bone : m_Bones)
		if (bone)
			delete bone;

	// animation은 animator에서 관리
	/*for (auto& animation : m_Animations)
		if (animation)
			delete animation;*/
}

Model* ModelLoader::Create(Renderer* renderer, const std::string& filePath, ModelParserFlags parserFlags /*= ModelParserFlags::NONE*/)
{
	Model* result = new Model(filePath);

	if (m_AssimpParser.LoadModel(renderer, filePath, result/*->m_Meshes, result->m_Nodes, result->m_Animations, result->m_MaterialNames*/, parserFlags))
		return result;

	delete result;

	return nullptr;
}

Model* ModelLoader::CreateForEntity(const std::string& filePath, ModelParserFlags parserFlags /*= ModelParserFlags::NONE*/)
{
	Model* result = new Model(filePath);

	if (m_AssimpParser.LoadModelForEntity(filePath, result, parserFlags))
		return result;
	else
	{
		delete result;
		return nullptr;
	}
}

void ModelLoader::Reload(Renderer* renderer, Model& model, const std::string& filePath, ModelParserFlags parserFlags /*= ModelParserFlags::NONE*/)
{
	Model* newModel = Create(renderer, filePath, parserFlags);

	if (newModel)
	{
		model.m_Meshes = newModel->m_Meshes;
		model.m_MaterialNames = newModel->m_MaterialNames;
		newModel->m_Meshes.clear();
		delete newModel;
	}
}

bool ModelLoader::Destroy(Model*& modelInstance)
{
	if (modelInstance)
	{
		delete modelInstance;
		modelInstance = nullptr;

		return true;
	}

	return false;
}

AnimationClip* ModelLoader::LoadAnimation(const std::string& filePath, ModelParserFlags parserFlags /*= ModelParserFlags::NONE*/)
{
	AnimationClip* result = new AnimationClip();

	if (m_AssimpParser.LoadAnimation(filePath, result, parserFlags))
		return result;

	delete result;

	return nullptr;
}

void ModelLoader::LoadAnimations(const std::string& filePath, std::vector<AnimationClip*>& animations, ModelParserFlags parserFlags /*= ModelParserFlags::NONE*/)
{
	m_AssimpParser.LoadAnimations(filePath, animations, parserFlags);
}

AssimpParser ModelLoader::m_AssimpParser;
