#pragma once
#include "AnimatorMeta.h"

namespace core
{
    class AnimatorParameter
	{
    public:
        AnimatorParameter() = default;

        AnimatorParameter(std::string name, entt::meta_any value)
            : name(std::move(name)), value(value) {}

        std::string name;
        entt::meta_any value;

        template <class Archive>
        void save(Archive& archive) const
    	{
            archive(CEREAL_NVP(name));
            ::save(archive, value);
        }

        template <class Archive>
        void load(Archive& archive)
    	{
            archive(CEREAL_NVP(name));
            ::load(archive, value);
        }
    };
}
