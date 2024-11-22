#pragma once

namespace core
{
	struct ButtonEvent
	{
		std::string functionName;
		std::vector<entt::meta_any> parameters;

		template<class Archive>
		void save(Archive& archive) const
		{
			archive(
				CEREAL_NVP(functionName)
			);

			uint32_t size = 0;
			for (auto& parameter : parameters)
				++size;

			archive(size);

			for (auto& parameter : parameters)
				::save(archive, parameter);

		}

		template<class Archive>
		void load(Archive& archive)
		{
			archive(
				CEREAL_NVP(functionName)
			);

			uint32_t size = 0;

			archive(size);

			parameters.resize(size);

			for (auto& parameter : parameters)
				::load(archive, parameter);
		}
	};
}