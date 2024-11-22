#pragma once
#include "TagAndLayerHelpers.h"

namespace tag
{
	// built-in tag
	DEFINE_TAG(Untagged);
	DEFINE_TAG(Respawn);
	DEFINE_TAG(Finish);
	DEFINE_TAG(MainCamera);
	DEFINE_TAG(Player);

}

namespace layer
{
	// built-in layer
	DEFINE_LAYER(Default, 0);
	DEFINE_LAYER(IgnoreRaycast, 1);
	DEFINE_LAYER(TransparentFX, 2);
	DEFINE_LAYER(UI, 3);
}

namespace core
{
	// 충돌 함수 콜백에 사용 (CollisionCallback 클래스 참조)
	struct Tag
	{
		entt::id_type id = tag::Untagged::id;
	};

	// 충돌 판정에 사용 (PhysicsScene 클래스 참조)
	struct Layer
	{
		entt::id_type mask = layer::Default::mask;
	};
}