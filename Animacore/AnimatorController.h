#pragma once
#include "RenderComponents.h"

namespace core
{
	// Animator 관리 객체
	class AnimatorController
	{
		using StateMap = Animator::StateMap;

	public:
		AnimatorController() = default;

		static void Save(const std::string& filePath, AnimatorController& controller);
		static AnimatorController Load(const std::string& filePath);

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(
				CEREAL_NVP(animatorName),
				CEREAL_NVP(entryStateName),
				CEREAL_NVP(stateMap),
				CEREAL_NVP(parameters)
			);
		}


		std::string animatorName = "New Animator";
		std::string entryStateName;

		void AddState(const std::string& stateName = "")
		{
			stateMap[stateName] = {};
			stateMap[stateName].name = stateName;
		}

		void RemoveState(const std::string& stateName)
		{
			stateMap.erase(stateName);
		}

		void AddParameter(const std::string& name, const entt::meta_any& value)
		{
			int index = 0;
			std::string newName = name;
			while (parameters.find(newName) != parameters.end())
			{
				newName = name + " " + std::to_string(index++);
			}
			parameters.emplace(newName, AnimatorParameter(newName, value));
		}

		std::unordered_map<std::string, AnimatorParameter> parameters;

		StateMap stateMap;
	};
}

