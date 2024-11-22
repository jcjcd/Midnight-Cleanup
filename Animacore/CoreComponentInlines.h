#pragma once
#include "RenderComponents.h"
#include "CorePhysicsComponents.h"

namespace core
{
	inline Rigidbody::Constraints operator|(Rigidbody::Constraints lhs, Rigidbody::Constraints rhs)
	{
		return static_cast<Rigidbody::Constraints>(
			static_cast<std::underlying_type_t<Rigidbody::Constraints>>(lhs) |
			static_cast<std::underlying_type_t<Rigidbody::Constraints>>(rhs));
	}

	inline Rigidbody::Constraints operator&(Rigidbody::Constraints lhs, Rigidbody::Constraints rhs)
	{
		return static_cast<Rigidbody::Constraints>(
			static_cast<std::underlying_type_t<Rigidbody::Constraints>>(lhs) &
			static_cast<std::underlying_type_t<Rigidbody::Constraints>>(rhs));
	}

	inline Rigidbody::Constraints operator^(Rigidbody::Constraints lhs, Rigidbody::Constraints rhs)
	{
		return static_cast<Rigidbody::Constraints>(
			static_cast<std::underlying_type_t<Rigidbody::Constraints>>(lhs) ^
			static_cast<std::underlying_type_t<Rigidbody::Constraints>>(rhs));
	}

	inline Rigidbody::Constraints operator~(Rigidbody::Constraints rhs)
	{
		return static_cast<Rigidbody::Constraints>(
			~static_cast<std::underlying_type_t<Rigidbody::Constraints>>(rhs));
	}

	inline Rigidbody::Constraints& operator|=(Rigidbody::Constraints& lhs, Rigidbody::Constraints rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	inline Rigidbody::Constraints& operator&=(Rigidbody::Constraints& lhs, Rigidbody::Constraints rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	inline Rigidbody::Constraints& operator^=(Rigidbody::Constraints& lhs, Rigidbody::Constraints rhs)
	{
		lhs = lhs ^ rhs;
		return lhs;
	}

	inline RenderAttributes::Flag operator|(const RenderAttributes::Flag& lhs, const RenderAttributes::Flag& rhs)
	{
		return static_cast<RenderAttributes::Flag>(
			static_cast<std::underlying_type_t<RenderAttributes::Flag>>(lhs) |
			static_cast<std::underlying_type_t<RenderAttributes::Flag>>(rhs));
	}

	inline RenderAttributes::Flag operator&(const RenderAttributes::Flag& lhs, const RenderAttributes::Flag& rhs)
	{
		return static_cast<RenderAttributes::Flag>(
			static_cast<std::underlying_type_t<RenderAttributes::Flag>>(lhs) &
			static_cast<std::underlying_type_t<RenderAttributes::Flag>>(rhs));
	}

	inline RenderAttributes::Flag operator^(const RenderAttributes::Flag& lhs, const RenderAttributes::Flag& rhs)
	{
		return static_cast<RenderAttributes::Flag>(
			static_cast<std::underlying_type_t<RenderAttributes::Flag>>(lhs) ^
			static_cast<std::underlying_type_t<RenderAttributes::Flag>>(rhs));
	}

	inline RenderAttributes::Flag operator~(RenderAttributes::Flag rhs)
	{
		// rhs를 기본 정수형으로 변환하여 비트 반전 후 다시 열거형으로 변환
		return static_cast<RenderAttributes::Flag>(
			~static_cast<std::underlying_type_t<RenderAttributes::Flag>>(rhs));
	}

	inline RenderAttributes::Flag operator|= (RenderAttributes::Flag& lhs, RenderAttributes::Flag rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	inline RenderAttributes::Flag operator&= (RenderAttributes::Flag& lhs, RenderAttributes::Flag rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	inline RenderAttributes::Flag operator^= (RenderAttributes::Flag& lhs, RenderAttributes::Flag rhs)
	{
		lhs = lhs ^ rhs;
		return lhs;
	}

}
