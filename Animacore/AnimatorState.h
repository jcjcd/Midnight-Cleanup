#pragma once
#include "AnimatorMeta.h"
#include "AnimatorTransition.h"


struct AnimationClip;

namespace core
{
	struct Animator;

	class AnimationEvent
	{
	public:
		float time = 0.0f;
		std::string functionName;
		//std::function<void(entt::meta_any)> function;
		//entt::meta_any parameter;
		std::vector<entt::meta_any> parameters;

		// 내부 처리 되었는가
		bool isProcessed = false;

		template<class Archive>
		void save(Archive& archive) const
		{
			archive(
				CEREAL_NVP(time),
				CEREAL_NVP(functionName)
			);

			uint32_t size = 0;
			for (auto& parameter : parameters)
				++size;
			
			archive(size);

			for (auto& parameter : parameters)
				::save(archive, parameter);

		}

		template<class Archive>
		void load(Archive& archive)
		{
			archive(
				CEREAL_NVP(time),
				CEREAL_NVP(functionName)
			);

			uint32_t size = 0;

			archive(size);

			parameters.resize(size);

			for (auto& parameter : parameters)
				::load(archive, parameter);
		}
	};

	class AnimatorState
	{
	public:
		//         // 멤버 변수 정보
		//         template<class T>
		// 		T& GetParameter(const std::string& name)
		// 		{
		//             assert(_animator);
		//             assert(_animator->parameters.contains(name));
		//             return _animator->parameters[name].value.try_cast<T&>();
		// 		}

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(
				CEREAL_NVP(name),
				CEREAL_NVP(transitions),
				CEREAL_NVP(motionName),
				CEREAL_NVP(multiplier),
				CEREAL_NVP(isLoop),
				CEREAL_NVP(animationEvents)
			);
		}

		std::string name;
		std::vector<AnimatorTransition> transitions;

		std::string motionName;
		std::shared_ptr<AnimationClip> motion;

		float multiplier = 1.0f;

		std::vector<AnimationEvent> animationEvents;

		bool isLoop = false;

	protected:
		Animator* _animator = nullptr;

		friend class AnimatorSystem;
	};




}


