#pragma once
#include <type_traits>

// Enum class에 대해 비트 연산자를 정의합니다.
template<typename Enum>
constexpr std::enable_if_t<std::is_enum_v<Enum>, Enum>
operator|(Enum lhs, Enum rhs) {
	using underlying = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template<typename Enum>
constexpr std::enable_if_t<std::is_enum_v<Enum>, Enum>
operator&(Enum lhs, Enum rhs) {
	using underlying = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template<typename Enum>
constexpr std::enable_if_t<std::is_enum_v<Enum>, Enum&>
operator&=(Enum& lhs, Enum rhs) {
	using underlying = std::underlying_type_t<Enum>;
	lhs = static_cast<Enum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
	return lhs;
}

template<typename Enum>
constexpr std::enable_if_t<std::is_enum_v<Enum>, Enum>
operator~(Enum rhs) {
	using underlying = std::underlying_type_t<Enum>;
	return static_cast<Enum>(~static_cast<underlying>(rhs));
}
