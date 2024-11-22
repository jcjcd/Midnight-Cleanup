#pragma once
#include "AnimatorMeta.h"

namespace core
{
    class AnimatorCondition
    {
    public:
        enum class Mode
        {
            If,
            IfNot,
            Greater,
            Less,
            Equals,
            NotEqual,
            Trigger
        };

        AnimatorCondition() = default;

        AnimatorCondition(Mode mode, std::string parameter, const entt::meta_any& threshold)
            : mode(mode), parameter(std::move(parameter)), threshold(threshold) {}

        Mode mode;
        std::string parameter;
        entt::meta_any threshold;


        template <class Archive>
        void save(Archive& archive) const
    	{
            archive(
                CEREAL_NVP(mode),
                CEREAL_NVP(parameter)
            );
            ::save(archive, threshold);
        }

        template <class Archive>
        void load(Archive& archive)
    	{
            archive(
                CEREAL_NVP(mode),
                CEREAL_NVP(parameter)
            );
            ::load(archive, threshold);
        }
    };
}

