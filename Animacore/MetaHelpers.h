// ReSharper disable CppClangTidyClangDiagnosticUnusedMacros
#pragma once

template<typename... Components>
constexpr std::vector<entt::id_type> GetComponentIDs() {
	return { entt::resolve<Components>(global::componentMetaCtx).id()... };
}

#define SET_PROP(prop_name, value)\
	.prop(prop_name##_hs, ##value)\

#define SET_TYPE(class)\
	.type(entt::type_hash<class>::value())\

#define SET_CONV(func)\
	.conv<func>()\

#define SET_NAME(class)\
	SET_PROP("name", #class)\

#define SET_DESC(description)\
	SET_PROP("description", u8##description)\

#define SET_MIN_MAX(min, max)\
	SET_PROP("min", min)\
	SET_PROP("max", max)\

#define SET_DRAG_SPEED(dragSpeed)\
	SET_PROP("dragSpeed", dragSpeed)\

#define SET_CATEGORY(category)\
	SET_PROP("category", static_cast<uint32_t>(category))\

#define SET_VALUE(value, name)\
	.data<value>(name##_hs)\
	SET_PROP("name", name)\
	SET_PROP("value", value)\

#define SET_MEMBER(memberAddress, name)\
	.data<&##memberAddress>(name##_hs)\
	SET_PROP("name", name)\

#define REGISTER_COMPONENT_META(class)\
	entt::meta<class>(global::componentMetaCtx)\
	SET_TYPE(class)\

#define SET_DEFAULT_FUNC(class)\
	.func<&core::GetName<class>>("GetName"_hs)\
	.func<&core::Assign<class>>("Assign"_hs)\
	.func<&core::GetComponent<class>>("GetComponent"_hs)\
	.func<&core::SetComponent<class>>("SetComponent"_hs)\
	.func<&core::GetHandle<class>>("GetHandle"_hs)\
	.func<&core::ResetComponent<class>>("ResetComponent"_hs)\
	.func<&core::AddComponent<class>>("AddComponent"_hs)\
	.func<&core::RemoveComponent<class>>("RemoveComponent"_hs)\
	.func<&core::HasComponent<class>>("HasComponent"_hs)\
	.func<&core::SaveSnapshot<class>>("SaveSnapshot"_hs)\
	.func<&core::LoadSnapshot<class>>("LoadSnapshot"_hs)\
	.func<&core::SavePrefabSnapshot<class>>("SavePrefabSnapshot"_hs)\
	.func<&core::LoadPrefabSnapshot<class>>("LoadPrefabSnapshot"_hs)\

#define IS_HIDDEN()\
	.prop("is_hidden"_hs)\

#define NEED_COMPONENTS(...) \
    SET_PROP("need_components", (::GetComponentIDs<__VA_ARGS__>()))\

#define REGISTER_CUSTOM_STRUCT_META(class)\
	entt::meta<class>(global::customStructMetaCtx)\
	SET_TYPE(class)\

#define REGISTER_CUSTOM_ENUM_META(enum)\
	entt::meta<enum>(global::customEnumMetaCtx)\
	SET_TYPE(enum)\

#define REGISTER_SYSTEM_META(class)\
	entt::meta<class>(global::systemMetaCtx)\
		SET_TYPE(class)\
		SET_NAME(class)\
		.func<&core::LoadSystem<class>>("LoadSystem"_hs)\
		.func<&core::RegisterSystem<class>>("RegisterSystem"_hs)\
		.func<&core::RemoveSystem<class>>("RemoveSystem"_hs)\

#define REGISTER_EVENT_META(class)\
	entt::meta<class>(global::eventMetaCtx)\
		SET_TYPE(class)\

#define REGISTER_TAG_META(class)\
	entt::meta<class>(global::tagMetaCtx)\
		SET_TYPE(class)\
		SET_NAME(class)\

#define REGISTER_LAYER_META(class)\
	entt::meta<class>(global::layerMetaCtx)\
		SET_TYPE(class)\
		SET_NAME(class)\
		SET_PROP("index", class::index)\
		SET_PROP("mask", class::mask);\
	core::AddLayer(#class, class::mask)\

#define SET_LAYER_COLLISION(layer1, layer2, isCollide)\
	core::SetLayerCollision(layer1::mask, layer2::mask, ##isCollide);

#define REGISTER_CALLBACK_EVENT_FUNC(function) \
	entt::meta<global::CallBackFuncDummy>(global::callbackEventMetaCtx)\
		.func<&(function)>(#function##_hs)\
		.prop("name"_hs, #function)