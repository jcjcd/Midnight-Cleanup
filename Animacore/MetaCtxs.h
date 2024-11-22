#pragma once

// 전역적으로 사용되는 메타 정보
namespace global
{
	inline entt::meta_ctx componentMetaCtx;
	inline entt::meta_ctx eventMetaCtx;
	inline entt::meta_ctx systemMetaCtx;
	inline entt::meta_ctx tagMetaCtx;
	inline entt::meta_ctx layerMetaCtx;
	inline entt::meta_ctx customStructMetaCtx;
	inline entt::meta_ctx customEnumMetaCtx;
	inline entt::meta_ctx callbackEventMetaCtx;
	class CallBackFuncDummy {};
}
