#pragma once

// meta_any 직렬화 함수
template <class Archive>
void save(Archive& archive, const entt::meta_any& any)
{
	if (!any) {
		auto typeHash = entt::id_type{};
		archive(CEREAL_NVP(typeHash));
		std::string value = std::string{};
		archive(CEREAL_NVP(value));
// 		archive(CEREAL_NVP(entt::id_type{}));
// 		archive(CEREAL_NVP(std::string{}));
		return;
	}

    auto typeHash = any.type().info().hash();
    archive(CEREAL_NVP(typeHash));

    if (any) {
        std::string value;

        if (any.type() == entt::resolve<int>()) {
            value = std::to_string(any.cast<int>());
        }
        else if (any.type() == entt::resolve<float>()) {
            value = std::to_string(any.cast<float>());
        }
        else if (any.type() == entt::resolve<bool>()) {
            value = any.cast<bool>() ? "true" : "false";
        }
        else if (any.type() == entt::resolve<std::string>()) {  
            value = any.cast<std::string>();
        }
        else if (any.type() == entt::resolve<entt::entity>()) {
            value = std::to_string((uint32_t)any.cast<entt::entity>());
        }

        archive(CEREAL_NVP(value));
    }
    else {
        archive(CEREAL_NVP(std::string{}));
    }
}

// meta_any 역직렬화 함수
template <class Archive>
void load(Archive& archive, entt::meta_any& any)
{
    entt::id_type typeHash;
    archive(CEREAL_NVP(typeHash));


    std::string value;
    archive(CEREAL_NVP(value));


    if (typeHash == entt::type_hash<int>::value()) {
		any = std::stoi(value);
	}
	else if (typeHash == entt::type_hash<float>::value()) {
		any = std::stof(value);
	}
	else if (typeHash == entt::type_hash<bool>::value()) {
		any = (value == "true");
	}
	else if (typeHash == entt::type_hash<std::string>::value()) {
		any = value;
	}
	else if (typeHash == entt::type_hash<entt::entity>::value()) {
		any = entt::entity(std::stoul(value));
	}
    else {
        any = entt::meta_any{};
    }
}