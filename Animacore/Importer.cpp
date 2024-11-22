#include "pch.h"
#include "Importer.h"

#include "Scene.h"
#include "CoreSerialize.h"
#include "CoreComponents.h"
#include "CoreSystemEvents.h"
#include "RenderComponents.h"

#include <fstream>

struct ObjectData
{
	std::string name;
	int instanceID;
	int parentID;
	Vector3 localPosition;
	Quaternion localRotation;
	Vector3 localScale;
	std::string meshName;
	std::vector<std::string> materialNames;
	int materialCount;
	int lightMapIndex;
	Vector4 lightMapScaleOffset;
	bool hasMeshCollider = false;
	bool hasBoxCollider = false;
	Vector3 boxColliderSize;
	Vector3 boxColliderOffset;

	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(
			CEREAL_NVP(name),
			CEREAL_NVP(instanceID),
			CEREAL_NVP(parentID),
			CEREAL_NVP(localPosition),
			CEREAL_NVP(localRotation),
			CEREAL_NVP(localScale),
			CEREAL_NVP(meshName),
			CEREAL_NVP(materialNames),
			CEREAL_NVP(materialCount),
			CEREAL_NVP(lightMapIndex),
			CEREAL_NVP(lightMapScaleOffset),
			CEREAL_NVP(hasMeshCollider),
			CEREAL_NVP(hasBoxCollider),
			CEREAL_NVP(boxColliderSize),
			CEREAL_NVP(boxColliderOffset)
		);
	}
};

void core::Importer::LoadUnityNew(const std::filesystem::path& path, core::Scene& scene)
{
	scene.Clear();
	LoadUnityContinuous(path, scene);
}

void core::Importer::LoadUnityContinuous(const std::filesystem::path& path, core::Scene& scene)
{
	std::ifstream file(path);
	cereal::JSONInputArchive archive(file);

	std::vector<ObjectData> objectDataList;

	archive(cereal::make_nvp("objects", objectDataList));

	std::unordered_map<int, core::Entity> idToEntityMap;

	// 모든 엔티티를 생성하고 매핑합니다.
	for (const auto& data : objectDataList)
	{
		core::Entity entity = scene.CreateEntity();
		idToEntityMap.insert({ data.instanceID, entity });

		// 이름 설정
		auto& name = entity.GetOrEmplace<core::Name>();
		name.name = data.name;

		// Transform 정보 설정
		auto& local = entity.GetOrEmplace<core::LocalTransform>();
		local.position = data.localPosition;
		local.rotation = data.localRotation;
		local.scale = data.localScale;

		entity.GetOrEmplace<core::WorldTransform>();

		// 메쉬 설정
		if (!data.meshName.empty())
		{
			auto& meshRenderer = entity.GetOrEmplace<core::MeshRenderer>();
			meshRenderer.meshString = data.meshName;

			for (int i = 0; i < data.materialCount; ++i)
			{
				//./Resources/Materials/Skin.material 이 형식으로 만들어야됨
				std::string materialPath = "./Resources/Materials/" + data.materialNames[i] + ".material";
				meshRenderer.materialStrings.push_back(materialPath);
			}
		}

		// 라이트맵 정보 설정
		if (data.lightMapIndex != -1)
		{
			auto& lightMap = entity.GetOrEmplace<core::LightMap>();
			lightMap.index = data.lightMapIndex;

			lightMap.tilling.x = data.lightMapScaleOffset.x;
			lightMap.tilling.y = data.lightMapScaleOffset.y;
			lightMap.offset.x = data.lightMapScaleOffset.z;
			lightMap.offset.y = data.lightMapScaleOffset.w;
		}



		// 콜라이더 정보 설정
		if (data.hasMeshCollider)
		{
			entity.Emplace<core::ColliderCommon>();
			auto& meshCollider = entity.Emplace<core::MeshCollider>();
			meshCollider.convex = false;
		}
		if (data.hasBoxCollider)
		{
			auto& common = entity.Emplace<core::ColliderCommon>();
			auto& boxCollider = entity.Emplace<core::BoxCollider>();
			boxCollider.size = data.boxColliderSize;
			common.center = data.boxColliderOffset;
		}
	}

	// 부모-자식 관계 설정
	for (const auto& data : objectDataList)
	{
		if (data.parentID != -1)
		{
			if (idToEntityMap.contains(data.instanceID) and idToEntityMap.contains(data.parentID))
			{
				core::Entity childEntity = idToEntityMap.find(data.instanceID)->second;
				core::Entity parentEntity = idToEntityMap.find(data.parentID)->second;
				childEntity.SetParent(parentEntity);
			}
			else
			{

				scene.GetDispatcher()->enqueue<OnThrow>(
					OnThrow::Error,
					"Parsing Error : Can not convert ID({}, {}) to Entity",
					data.instanceID, data.parentID);
			}
		}
	}
}
