#pragma once
#include "AnimatorCondition.h"


namespace core
{
	class AnimatorState;

    class AnimatorTransition
	{
    public:
        std::string destinationState;
        std::vector<AnimatorCondition> conditions;
		float blendTime = 0.0f;
        float exitTime = 0.0f;

        void AddCondition(AnimatorCondition::Mode mode, const std::string& parameter, const entt::meta_any& threshold)
    	{
            conditions.emplace_back(mode, parameter, threshold);
        }

        void RemoveCondition(size_t index)
    	{
            if (index < conditions.size()) {
                conditions.erase(conditions.begin() + index);
            }
        }

        template<class Archive>
        void serialize(Archive& archive)
    	{
            archive(destinationState, conditions, blendTime, exitTime);
        }
    };
}

