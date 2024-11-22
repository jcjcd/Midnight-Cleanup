#include "pch.h"
#include "AssimpParser.h"
#include "Mesh.h"
#include "Utility.h"
#include "ModelLoader.h"
#include "Material.h"

static Matrix ConvertMatrix(const aiMatrix4x4& aiMat);
static void ConvertUpVector(aiScene* scene);
static void ConvertNodeTransform(aiNode* node, const Matrix& convertMatrix);

bool AssimpParser::LoadModelForEntity(const std::string& fileName, Model* model, ModelParserFlags parserFlags)
{
	Assimp::Importer import;
	import.SetPropertyInteger(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);
	const aiScene * scene = import.ReadFile(fileName, static_cast<unsigned int>(parserFlags));

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		return false;

	aiScene* curScene = const_cast<aiScene*>(scene);

	aiMatrix4x4 identity;

	processNodeForEntity(&identity, curScene->mRootNode, curScene, nullptr, &model->m_EntityRootNode);


import.FreeScene();

	return true;
}


void AssimpParser::processNode2(Renderer* renderer, void* transform, struct aiNode* node, const struct aiScene* scene, std::vector<Mesh*>& meshes)
{
	aiMatrix4x4 nodeTransform = *reinterpret_cast<aiMatrix4x4*>(transform) * node->mTransformation;

	Node* newNode = new Node;
	newNode->childCount = node->mNumChildren;
	newNode->children.resize(node->mNumChildren);

	if (node->mNumMeshes > 0)
	{
		Mesh* newMesh = processMesh2(renderer, &nodeTransform, scene, node);
		newMesh->name = node->mName.C_Str();
		meshes.push_back(newMesh);

		newNode->mesh = newMesh;

		Matrix transform(node->mTransformation[0]);
		newNode->toRootTransform = transform;

		Matrix parentTransform = Matrix::Identity;
		if (node->mParent)
		{
			parentTransform = Matrix(node->mParent->mTransformation[0]);
		}
		newNode->toRootTransform = transform * parentTransform;
	}
	// node의 모든 mesh들을 처리


	// node의 모든 자식 노드들을 처리
	for (uint32_t i = 0; i < node->mNumChildren; i++)
		processNode2(renderer, &nodeTransform, node->mChildren[i], scene, meshes);
}

Mesh* AssimpParser::processMesh2(Renderer* renderer, void* transform, const struct aiScene* scene, const aiNode* node)
{
	aiMatrix4x4 meshTransform = *reinterpret_cast<aiMatrix4x4*>(transform);


	Mesh* newMesh = new Mesh;
	newMesh->name = node->mName.C_Str();
	newMesh->subMeshCount = node->mNumMeshes;
	newMesh->subMeshDescriptors.reserve(node->mNumMeshes);

	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;
	uint32_t boneCount = 0;

	bool hasNormals = false;
	bool hasUV = false;
	bool hasUV2 = false;
	bool hasTangents = false;
	bool hasBitangents = false;
	bool hasBones = false;

	std::vector<DirectX::BoundingBox> boundingBoxes;

	for (uint32_t i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

		SubMeshDescriptor descriptor;
		descriptor.name = mesh->mName.C_Str();
		descriptor.indexCount = mesh->mNumFaces * 3;
		descriptor.indexOffset = indexCount;
		descriptor.vertexCount = mesh->mNumVertices;
		descriptor.vertexOffset = vertexCount;

		newMesh->subMeshDescriptors.push_back(descriptor);

		vertexCount += mesh->mNumVertices;
		indexCount += mesh->mNumFaces * 3;
		boneCount = mesh->mNumBones;

		if (hasNormals == false && mesh->mNormals != nullptr)
			hasNormals = true;

		if (hasUV == false && mesh->mTextureCoords[0] != nullptr)
			hasUV = true;

		if (hasUV2 == false && mesh->mTextureCoords[1] != nullptr)
			hasUV2 = true;

		if (hasTangents == false && mesh->mTangents != nullptr)
			hasTangents = true;

		if (hasBitangents == false && mesh->mBitangents != nullptr)
			hasBitangents = true;

		if (hasBones == false && mesh->mNumBones > 0)
			hasBones = true;

		DirectX::BoundingBox subMeshBoundingBox;
		auto centerDouble = (mesh->mAABB.mMin + mesh->mAABB.mMax);
		auto extentsDouble = (mesh->mAABB.mMax - mesh->mAABB.mMin);
		subMeshBoundingBox.Center = Vector3(centerDouble.x, centerDouble.y, centerDouble.z) * 0.5f;
		subMeshBoundingBox.Extents = Vector3(extentsDouble.x, extentsDouble.y, extentsDouble.z) * 0.5f;

		boundingBoxes.push_back(subMeshBoundingBox);
	}

	for (auto& box : boundingBoxes)
		DirectX::BoundingBox::CreateMerged(newMesh->boundingBox, newMesh->boundingBox, box);

	// 바운딩스피어도 만든다.
	DirectX::BoundingSphere::CreateFromBoundingBox(newMesh->boundingSphere, newMesh->boundingBox);

	newMesh->vertices.resize(vertexCount, {});
	if (hasNormals)
		newMesh->normals.resize(vertexCount, {});
	if (hasUV)
		newMesh->uv.resize(vertexCount, {});
	if (hasUV2)
		newMesh->uv2.resize(vertexCount, {});
	if (hasTangents)
		newMesh->tangents.resize(vertexCount, {});
	if (hasBitangents)
		newMesh->bitangents.resize(vertexCount, {});
	if (hasBones)
		newMesh->boneWeights.resize(boneCount, {});

	newMesh->indices.resize(indexCount);

	std::set<int> boneIndices;

	for (uint32_t i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		SubMeshDescriptor& descriptor = newMesh->subMeshDescriptors[i];

		for (uint32_t j = 0; j < mesh->mNumVertices; j++)
		{
			aiVector3D position = meshTransform * mesh->mVertices[j];
			newMesh->vertices[descriptor.vertexOffset + j] = Vector3(position.x, position.y, position.z);

			if (mesh->mNormals != nullptr)
			{
				aiVector3D normal = meshTransform * mesh->mNormals[j];
				newMesh->normals[descriptor.vertexOffset + j] = Vector3(normal.x, normal.y, normal.z);
			}
		}

		if (mesh->mTextureCoords[0] != nullptr)
		{
			for (uint32_t j = 0; j < mesh->mNumVertices; j++)
			{
				newMesh->uv[descriptor.vertexOffset + j] = Vector2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
			}
		}

		if (mesh->mTextureCoords[1] != nullptr)
		{
			for (uint32_t j = 0; j < mesh->mNumVertices; j++)
			{
				newMesh->uv2[descriptor.vertexOffset + j] = Vector2(mesh->mTextureCoords[1][j].x, mesh->mTextureCoords[1][j].y);
			}
		}

		if (mesh->mTangents != nullptr)
			memcpy(&newMesh->tangents[descriptor.vertexOffset], mesh->mTangents, sizeof(Vector3) * mesh->mNumVertices);


		if (mesh->mBitangents != nullptr)
			memcpy(&newMesh->bitangents[descriptor.vertexOffset], mesh->mBitangents, sizeof(Vector3) * mesh->mNumVertices);

		if (mesh->HasBones())
		{
			std::vector<std::vector<std::pair<uint32_t, float>>> vertexWeights(vertexCount);
			newMesh->boneWeights.resize(vertexCount);

			for (uint32_t j = 0; j < mesh->mNumBones; j++)
			{
				aiBone* curBone = mesh->mBones[j];

				for (uint32_t k = 0; k < curBone->mNumWeights; k++)
				{
					uint32_t vertexID = curBone->mWeights[k].mVertexId + descriptor.vertexOffset;
					float weight = curBone->mWeights[k].mWeight;
					vertexWeights[vertexID].push_back({ j,weight });
				}

				if (curBone->mNumWeights)
				{
					int a = 0;
				}

				if (boneIndices.find(j) != boneIndices.end())
				{
					int a = 0;
				}

				boneIndices.insert(j);

				descriptor.boneIndexMap[curBone->mName.C_Str()].first = j;
				descriptor.boneIndexMap[curBone->mName.C_Str()].second = ConvertMatrix(curBone->mOffsetMatrix);
// 				newMesh->boneIndexMap[curBone->mName.C_Str()].first = j;
// 				newMesh->boneIndexMap[curBone->mName.C_Str()].second = ConvertMatrix(curBone->mOffsetMatrix);
			}

			for (uint32_t v = descriptor.vertexOffset; v < vertexCount; v++)
			{
				sort(vertexWeights[v].begin(), vertexWeights[v].end(), [](const auto& a, const auto& b) {
					return a.second > b.second;
					});

				float weightSum = 0.0f;

				for (uint32_t k = 0; k < 4 && k < vertexWeights[v].size(); k++)
					weightSum += vertexWeights[v][k].second;

				BoneWeight boneWeight = {};
				for (uint32_t k = 0; k < 4 && k < vertexWeights[v].size(); k++)
				{
					boneWeight.boneIndex[k] = vertexWeights[v][k].first;
					boneWeight.weight[k] = vertexWeights[v][k].second / weightSum;
				}
				newMesh->boneWeights[v] = boneWeight;
			}
		}

		for (uint32_t j = 0; j < mesh->mNumFaces; j++)
		{
			aiFace face = mesh->mFaces[j];
			memcpy(&newMesh->indices[descriptor.indexOffset + j * 3], face.mIndices, sizeof(uint32_t) * 3);
		}
	}

	//newMesh->CreateBuffers(renderer);

	return newMesh;
}


void AssimpParser::processAnimations(const struct aiScene* scene, std::vector<AnimationClip*>& animations, const std::string& filePath)
{
	if (scene->mNumAnimations == 0)
		return;

	std::string name = filePath.substr(filePath.find_last_of('_') + 1);
	name = name.substr(0, name.find_last_of('.'));

	animations.reserve(scene->mNumAnimations);

	for (uint32_t a = 0; a < scene->mNumAnimations; a++)
	{
		auto animation = new AnimationClip;
		aiAnimation* aiAnim = scene->mAnimations[a];

		animation->name = name;
		animation->framePerSecond = static_cast<float>(aiAnim->mTicksPerSecond);
		animation->frameCount = static_cast<uint32_t>(aiAnim->mDuration);
		animation->duration = animation->frameCount / animation->framePerSecond;
		animation->nodeClips.reserve(aiAnim->mNumChannels);

		for (uint32_t c = 0; c < aiAnim->mNumChannels; c++)
		{
			aiNodeAnim* nodeAnim = aiAnim->mChannels[c];
			auto nodeClip = std::make_shared<NodeClip>();
			nodeClip->nodeName = nodeAnim->mNodeName.C_Str();
			nodeClip->keyframes.reserve(nodeAnim->mNumPositionKeys);

			assert(nodeAnim->mNumPositionKeys == nodeAnim->mNumRotationKeys && nodeAnim->mNumPositionKeys == nodeAnim->mNumScalingKeys);

			for (uint32_t k = 0; k < nodeAnim->mNumPositionKeys; k++)
			{
				assert(nodeAnim->mPositionKeys[k].mTime == nodeAnim->mRotationKeys[k].mTime && nodeAnim->mPositionKeys[k].mTime == nodeAnim->mScalingKeys[k].mTime);

				auto translation = nodeAnim->mPositionKeys[k].mValue;
				auto rotation = nodeAnim->mRotationKeys[k].mValue;
				auto scale = nodeAnim->mScalingKeys[k].mValue;

				Keyframe data;
				data.timePos = static_cast<float>(nodeAnim->mPositionKeys[k].mTime) / animation->framePerSecond;
				data.translation = { translation.x, translation.y, translation.z };
				data.rotation = { rotation.x, rotation.y, rotation.z, rotation.w };
				data.scale = { scale.x, scale.y, scale.z };
				nodeClip->keyframes.push_back(std::move(data));
			}

			if (nodeClip->keyframes.size() < animation->frameCount)
			{
				uint32_t count = animation->frameCount - static_cast<uint32_t>(nodeClip->keyframes.size());

				for (uint32_t i = 0; i < count; i++)
					nodeClip->keyframes.push_back(nodeClip->keyframes.back());
			}

			animation->nodeClips.push_back(nodeClip);
		}
		animations.push_back(animation);
	}
}

void AssimpParser::processNode3(Renderer* renderer, void* transform, struct aiNode* node, const struct aiScene* scene, std::vector<Mesh*>& meshes, std::vector<Node*>& nodes, Node* parent)
{
	aiMatrix4x4 nodeTransform = *reinterpret_cast<aiMatrix4x4*>(transform) * node->mTransformation;

	Node* newNode = new Node;
	newNode->childCount = node->mNumChildren;
	newNode->name = node->mName.C_Str();

	Matrix nt(node->mTransformation[0]);
	newNode->toRootTransform = nt;

	Matrix parentTransform = Matrix::Identity;
	if (node->mParent)
	{
		parentTransform = Matrix(node->mParent->mTransformation[0]);
		parent->children.emplace_back(newNode);
		newNode->parent = parent;
	}

	newNode->toRootTransform = nt * parentTransform;

	if (node->mNumMeshes > 0)
	{
		Mesh* newMesh = processMesh2(renderer, &nodeTransform, scene, node);
		meshes.push_back(newMesh);

		newNode->mesh = newMesh;
		newNode->subMeshCount++;
	}

	nodes.emplace_back(newNode);

	// node의 모든 자식 노드들을 처리
	for (uint32_t i = 0; i < node->mNumChildren; i++)
		processNode3(renderer, &nodeTransform, node->mChildren[i], scene, meshes, nodes, newNode);

}

void AssimpParser::processNode4(Renderer* renderer, void* transform, struct aiNode* node, const struct aiScene* scene, std::vector<Mesh*>& meshes, std::vector<Node*>& nodes, Node* parent, uint32_t parentIndex)
{
	aiMatrix4x4 nodeTransform = *reinterpret_cast<aiMatrix4x4*>(transform);

	Node* newNode = new Node;
	newNode->childCount = node->mNumChildren;
	newNode->name = node->mName.C_Str();
	newNode->index = static_cast<uint32_t>(nodes.size());
	newNode->parentIndex = parentIndex;

	newNode->toRootTransform = ConvertMatrix(nodeTransform);

	if (node->mParent)
	{
		newNode->parent = parent;
		parent->children.emplace_back(newNode);
	}

	if (node->mNumMeshes > 0)
	{
		Mesh* newMesh = processMesh2(renderer, &nodeTransform, scene, node);
		meshes.push_back(newMesh);

		newNode->mesh = newMesh;
		//newNode->subMeshCount++;
	}

	nodes.emplace_back(newNode);

	// node의 모든 자식 노드들을 처리
	for (uint32_t i = 0; i < node->mNumChildren; i++)
		processNode4(renderer, &nodeTransform, node->mChildren[i], scene, meshes, nodes, newNode, newNode->index);
}

void AssimpParser::processBone(const struct aiScene* scene, std::vector<Bone*>& bones)
{
	for (uint32_t i = 0; i < scene->mNumMeshes; i++)
	{
		auto mesh = scene->mMeshes[i];
		for (uint32_t j = 0; j < mesh->mNumBones; j++)
		{
			auto bone = mesh->mBones[j];

			Bone* newBonePtr = new Bone;
			newBonePtr->name = bone->mName.C_Str();
			newBonePtr->offsetMatrix = ConvertMatrix(bone->mOffsetMatrix);
			bones.emplace_back(newBonePtr);
		}
	}
}

void AssimpParser::processNodeForEntity(void* transform, struct aiNode* node, const struct aiScene* scene, EntityNode* parent, EntityNode** root)
{
	aiMatrix4x4 nodeTransform = *reinterpret_cast<aiMatrix4x4*>(transform) * node->mTransformation;

	EntityNode* newNode = new EntityNode;

	if (parent == nullptr)
		*root = newNode;

	newNode->childCount = node->mNumChildren;
	newNode->name = node->mName.C_Str();

	newNode->NodeTransform = ConvertMatrix(node->mTransformation);

	if (node->mParent)
	{
		newNode->parent = parent;
		parent->children.emplace_back(newNode);
	}

	if (node->mNumMeshes > 0)
	{
		newNode->meshCount = node->mNumMeshes;
		newNode->isSkinned = scene->mMeshes[node->mMeshes[0]]->HasBones();
	}

	// node의 모든 자식 노드들을 처리
	for (uint32_t i = 0; i < node->mNumChildren; i++)
		processNodeForEntity(&nodeTransform, node->mChildren[i], scene, newNode, nullptr);
}

bool AssimpParser::LoadAnimation(const std::string& fileName, AnimationClip* animationClip, ModelParserFlags flag)
{
	Assimp::Importer import;
import.SetPropertyInteger(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);
	const aiScene * scene = import.ReadFile(fileName, static_cast<unsigned int>(flag));

	if (!scene || !scene->mRootNode)
		return false;

	if (scene->mNumAnimations == 0)
		return false;

	std::string name = fileName.substr(fileName.find_last_of('_') + 1);
	name = name.substr(0, name.find_last_of('.'));

	for (uint32_t a = 0; a < scene->mNumAnimations; a++)
	{
		aiAnimation* aiAnim = scene->mAnimations[a];

		animationClip->name = name;
		animationClip->framePerSecond = static_cast<float>(aiAnim->mTicksPerSecond);
		animationClip->frameCount = static_cast<uint32_t>(aiAnim->mDuration);
		animationClip->duration = animationClip->frameCount / animationClip->framePerSecond;
		animationClip->nodeClips.reserve(aiAnim->mNumChannels);

		for (uint32_t c = 0; c < aiAnim->mNumChannels; c++)
		{
			aiNodeAnim* nodeAnim = aiAnim->mChannels[c];
			auto nodeClip = std::make_shared<NodeClip>();
			nodeClip->nodeName = nodeAnim->mNodeName.C_Str();
			nodeClip->keyframes.reserve(nodeAnim->mNumPositionKeys);

			assert(nodeAnim->mNumPositionKeys == nodeAnim->mNumRotationKeys && nodeAnim->mNumPositionKeys == nodeAnim->mNumScalingKeys);

			for (uint32_t k = 0; k < nodeAnim->mNumPositionKeys; k++)
			{
				assert(nodeAnim->mPositionKeys[k].mTime == nodeAnim->mRotationKeys[k].mTime && nodeAnim->mPositionKeys[k].mTime == nodeAnim->mScalingKeys[k].mTime);

				auto translation = nodeAnim->mPositionKeys[k].mValue;
				auto rotation = nodeAnim->mRotationKeys[k].mValue;
				auto scale = nodeAnim->mScalingKeys[k].mValue;

				Keyframe data;
				data.timePos = static_cast<float>(nodeAnim->mPositionKeys[k].mTime) / animationClip->framePerSecond;
				data.translation = { translation.x, translation.y, translation.z };
				data.rotation = { rotation.x, rotation.y, rotation.z, rotation.w };
				data.scale = { scale.x, scale.y, scale.z };
				nodeClip->keyframes.push_back(std::move(data));
			}

			if (nodeClip->keyframes.size() < animationClip->frameCount)
			{
				uint32_t count = animationClip->frameCount - static_cast<uint32_t>(nodeClip->keyframes.size());

				for (uint32_t i = 0; i < count; i++)
					nodeClip->keyframes.push_back(nodeClip->keyframes.back());
			}

			animationClip->nodeClips.push_back(nodeClip);
		}
	}

	return true;
}

bool AssimpParser::LoadAnimations(const std::string& fileName, std::vector<AnimationClip*>& animations, ModelParserFlags flag)
{
	Assimp::Importer import;
import.SetPropertyInteger(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);
	const aiScene * scene = import.ReadFile(fileName, static_cast<unsigned int>(flag));

	if (!scene || !scene->mRootNode)
		return false;

	if (scene->mNumAnimations == 0)
		return false;

	///std::string name = fileName.substr(fileName.find_last_of('_') + 1);
	///name = name.substr(0, name.find_last_of('.'));

	animations.reserve(scene->mNumAnimations);

	for (uint32_t a = 0; a < scene->mNumAnimations; a++)
	{
		auto animation = new AnimationClip;
		aiAnimation* aiAnim = scene->mAnimations[a];

		animation->name = scene->mAnimations[a]->mName.C_Str();
		animation->framePerSecond = static_cast<float>(aiAnim->mTicksPerSecond);
		animation->frameCount = static_cast<uint32_t>(aiAnim->mDuration);
		animation->duration = animation->frameCount / animation->framePerSecond;
		animation->nodeClips.reserve(aiAnim->mNumChannels);

		for (uint32_t c = 0; c < aiAnim->mNumChannels; c++)
		{
			aiNodeAnim* nodeAnim = aiAnim->mChannels[c];
			auto nodeClip = std::make_shared<NodeClip>();
			nodeClip->nodeName = nodeAnim->mNodeName.C_Str();
			nodeClip->keyframes.reserve(nodeAnim->mNumPositionKeys);

			assert(nodeAnim->mNumPositionKeys == nodeAnim->mNumRotationKeys && nodeAnim->mNumPositionKeys == nodeAnim->mNumScalingKeys);

			for (uint32_t k = 0; k < nodeAnim->mNumPositionKeys; k++)
			{
				assert(nodeAnim->mPositionKeys[k].mTime == nodeAnim->mRotationKeys[k].mTime && nodeAnim->mPositionKeys[k].mTime == nodeAnim->mScalingKeys[k].mTime);

				auto translation = nodeAnim->mPositionKeys[k].mValue;
				auto rotation = nodeAnim->mRotationKeys[k].mValue;
				auto scale = nodeAnim->mScalingKeys[k].mValue;

				// Quat에서 오일러 각도로 변환
				Quaternion quat(rotation.x, rotation.y, rotation.z, rotation.w);
				Vector3 euler = quat.ToEuler();  // YawPitchRoll 순으로 반환됩니다.

				// x축과 z축의 부호를 반전
// 				euler.x = -euler.x;
// 				euler.z = -euler.z;

				// 오일러 각도를 다시 쿼터니언으로 변환
				quat = Quaternion::CreateFromYawPitchRoll(euler.y, euler.x, euler.z);  // 순서에 유의


				Keyframe data;
				data.timePos = static_cast<float>(nodeAnim->mPositionKeys[k].mTime) / animation->framePerSecond;
				data.translation = { translation.x, translation.y, translation.z };
				//data.rotation = { rotation.x, rotation.y, rotation.z, rotation.w };
				data.rotation = quat;
				data.scale = { scale.x, scale.y, scale.z };
				nodeClip->keyframes.push_back(std::move(data));

			}

			if (nodeClip->keyframes.size() < animation->frameCount)
			{
				uint32_t count = animation->frameCount - static_cast<uint32_t>(nodeClip->keyframes.size());

				for (uint32_t i = 0; i < count; i++)
					nodeClip->keyframes.push_back(nodeClip->keyframes.back());
			}

			animation->nodeClips.push_back(nodeClip);
		}

		animations.push_back(animation);
	}

	return true;
}

bool AssimpParser::LoadModel(Renderer* renderer, const std::string& fileName, Model* model, ModelParserFlags parserFlags)
{
	Assimp::Importer import;
import.SetPropertyInteger(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);
	const aiScene * scene = import.ReadFile(fileName, static_cast<unsigned int>(parserFlags));

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		return false;

	aiScene* curScene = const_cast<aiScene*>(scene);
	/*Matrix rootTransform = ConvertMatrix(curScene->mRootNode->mTransformation);
	rootTransform = rootTransform * Matrix::CreateRotationX(DirectX::XM_PIDIV2);
	curScene->mRootNode->mTransformation = aiMatrix4x4(
		rootTransform._11, rootTransform._21, rootTransform._31, rootTransform._41,
		rootTransform._12, rootTransform._22, rootTransform._32, rootTransform._42,
		rootTransform._13, rootTransform._23, rootTransform._33, rootTransform._43,
		rootTransform._14, rootTransform._24, rootTransform._34, rootTransform._44
	);*/

	//ConvertUpVector(curScene);

	// WOO : embeded texture를 로드할 수 있도록 해야함
	//processMaterials(scene, model->GetMaterials());


	aiMatrix4x4 identity;

	processNode4(renderer, &identity, curScene->mRootNode, curScene, model->GetMeshes(), model->GetNodes(), nullptr, -1);
	processBone(curScene, model->GetBones());
	processAnimations(curScene, model->GetAnimations(), fileName);

import.FreeScene();

	return true;
}

static Matrix ConvertMatrix(const aiMatrix4x4& aiMat)
{
	return Matrix(
		aiMat.a1, aiMat.a2, aiMat.a3, aiMat.a4,
		aiMat.b1, aiMat.b2, aiMat.b3, aiMat.b4,
		aiMat.c1, aiMat.c2, aiMat.c3, aiMat.c4,
		aiMat.d1, aiMat.d2, aiMat.d3, aiMat.d4
	).Transpose();
}

static void ConvertUpVector(aiScene* scene)
{
	Matrix convertMatrix = Matrix::CreateRotationX(DirectX::XM_PIDIV2);

	//ConvertNodeTransform(scene->mRootNode, convertMatrix);

	for (uint32_t i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[i];

		// convert vertex
		for (uint32_t j = 0; j < mesh->mNumVertices; j++)
		{
			aiVector3D& pos = mesh->mVertices[j];
			Vector3 newPos = Vector3(pos.x, pos.y, pos.z);
			newPos = Vector3::Transform(newPos, convertMatrix);
			pos.x = newPos.x;
			pos.y = newPos.y;
			pos.z = newPos.z;
		}

		// convert normal
		if (mesh->HasNormals())
		{
			for (uint32_t j = 0; j < mesh->mNumVertices; j++)
			{
				aiVector3D& normal = mesh->mNormals[j];
				Vector3 newNormal = Vector3(normal.x, normal.y, normal.z);
				newNormal = Vector3::TransformNormal(newNormal, convertMatrix);
				normal.x = newNormal.x;
				normal.y = newNormal.y;
				normal.z = newNormal.z;
			}
		}

		// convert tangent
		if (mesh->HasTangentsAndBitangents())
		{
			for (uint32_t j = 0; j < mesh->mNumVertices; j++)
			{
				aiVector3D& tangent = mesh->mTangents[j];
				Vector3 newTangent = Vector3(tangent.x, tangent.y, tangent.z);
				newTangent = Vector3::TransformNormal(newTangent, convertMatrix);
				tangent.x = newTangent.x;
				tangent.y = newTangent.y;
				tangent.z = newTangent.z;

				aiVector3D& bitangent = mesh->mBitangents[j];
				Vector3 newBitangent = Vector3(bitangent.x, bitangent.y, bitangent.z);
				newBitangent = Vector3::TransformNormal(newBitangent, convertMatrix);
				bitangent.x = newBitangent.x;
				bitangent.y = newBitangent.y;
				bitangent.z = newBitangent.z;
			}
		}
	}

	// convert bone
	/*if (scene->HasAnimations())
	{
		for (uint32_t i = 0; i < scene->mNumAnimations; i++)
		{
			aiAnimation* anim = scene->mAnimations[i];
			for (uint32_t j = 0; j < anim->mNumChannels; j++)
			{
				aiNodeAnim* nodeAnim = anim->mChannels[j];
				for (uint32_t k = 0; k < nodeAnim->mNumPositionKeys; k++)
				{
					aiVector3D& pos = nodeAnim->mPositionKeys[k].mValue;
					Vector3 newPos = Vector3(pos.x, pos.y, pos.z);
					newPos = Vector3::Transform(newPos, convertMatrix);
					pos.x = newPos.x;
					pos.y = newPos.y;
					pos.z = newPos.z;
				}

				for (uint32_t k = 0; k < nodeAnim->mNumRotationKeys; k++)
				{
					aiQuaternion& rot = nodeAnim->mRotationKeys[k].mValue;
					Quaternion newRot = Quaternion(rot.x, rot.y, rot.z, rot.w);
					Quaternion rotationAdjustment = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), DirectX::XM_PIDIV2);
					newRot = newRot * rotationAdjustment;
					rot = aiQuaternion(newRot.w, newRot.x, newRot.y, newRot.z);
				}

				for (uint32_t k = 0; k < nodeAnim->mNumScalingKeys; k++)
				{
					aiVector3D& scale = nodeAnim->mScalingKeys[k].mValue;
					Vector3 newScale = Vector3(scale.x, scale.y, scale.z);
					newScale = Vector3::TransformNormal(newScale, convertMatrix);
					scale.x = newScale.x;
					scale.y = newScale.y;
					scale.z = newScale.z;
				}
			}
		}
	}*/
}

static void ConvertNodeTransform(aiNode* node, const Matrix& convertMatrix)
{
	aiMatrix4x4 nodeTransform = node->mTransformation;
	Matrix transform = Matrix(
		nodeTransform.a1, nodeTransform.a2, nodeTransform.a3, nodeTransform.a4,
		nodeTransform.b1, nodeTransform.b2, nodeTransform.b3, nodeTransform.b4,
		nodeTransform.c1, nodeTransform.c2, nodeTransform.c3, nodeTransform.c4,
		nodeTransform.d1, nodeTransform.d2, nodeTransform.d3, nodeTransform.d4
	);

	transform = transform * convertMatrix/* * convertMatrix.Invert()*/;
	nodeTransform = aiMatrix4x4(
		transform._11, transform._12, transform._13, transform._14,
		transform._21, transform._22, transform._23, transform._24,
		transform._31, transform._32, transform._33, transform._34,
		transform._41, transform._42, transform._43, transform._44
	).Transpose();

	node->mTransformation = nodeTransform;

	for (uint32_t i = 0; i < node->mNumChildren; i++)
		ConvertNodeTransform(node->mChildren[i], convertMatrix);
}