#pragma once

#define DEFINE_TAG(name) \
	struct name \
	{ \
		static constexpr entt::id_type id = entt::type_hash<tag::##name>().value(); \
	} \

#define DEFINE_LAYER(name, layerIndex) \
	struct name \
	{ \
		static constexpr uint32_t index = layerIndex; \
		static constexpr uint32_t mask = 1 << (layerIndex); \
	} \
