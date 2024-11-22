#pragma once
#include "Entity.h"

namespace physics
{
	// 레이캐스트를 위한 레이
	// 시작점과 방향을 가짐
	struct Ray
	{
		Vector3 origin; // 시작점
		Vector3 direction; // 방향

		Ray(const Vector3& origin, const Vector3& direction)
			: origin(origin), direction(direction) {}
	};

	// 레이캐스트의 결과를 가지는 구조체
	// 
	struct RaycastHit
	{
		entt::entity entity = entt::null;
		Vector3 point;
		Vector3 normal;
		float distance;

		// distance 기준으로 비교 연산자 오버로딩
		bool operator<(const RaycastHit& other) const
		{
			return distance < other.distance;
		}
	};
}

