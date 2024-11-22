#pragma once

#define DEFINE_SYSTEM_TRAITS(SystemClass) \
    template<> \
    struct SystemTraits<SystemClass> \
    { \
        static constexpr auto name = #SystemClass; \
    };

namespace core
{
	class ISystem;

	enum class SystemType
	{
		None,
		PreUpdate,
		Update,
		FixedUpdate,
		PreRender,
		Render,
		PostRender,
		CollisionHandler,
	};
}


template <typename T>
struct SystemTraits;

template <typename T>
concept HasSystemTraits = requires
{
	{ SystemTraits<T>::name } -> std::convertible_to<const char*>;
}&& std::is_base_of_v<core::ISystem, T>;