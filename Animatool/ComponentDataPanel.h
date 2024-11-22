#pragma once
#include "Panel.h"
#include "../Animacore/MetaCtxs.h"

#include <robin_hood.h>

class Renderer;

/// 클래스(네임스페이스) 하나 만들어서
/// 그 클래스가 버스트같이 특수화 해야하는 구조체를 다루는 static 함수들 관리하는 클래스
/// 해당 static 함수들 전부 이름 똑같이 해서
/// RenderStruct(Burst& burst)
/// RenderStruct(Explosion& explosion)

class Font;
class Texture;

namespace tool
{
	// concept 정의
	template <typename T>
	concept HasCStr = requires(T a)
	{
		{ a.c_str() } -> std::convertible_to<const char*>;
	};

	class ComponentDataPanel : public tool::Panel
	{
	public:
		explicit ComponentDataPanel(entt::dispatcher& dispatcher, Renderer& renderer);

		void RenderPanel(float deltaTime) override;
		PanelType GetType() override { return PanelType::Inspector; }

	private:
		// Add Component
		void showAddComponentPopup(entt::registry& registry);

		// 특정 확장자를 가진 파일들 획득
		std::vector<std::string> getFilesWithExtension(const std::string& directory, const std::string& extension, bool isFullName = false);

		// 컴포넌트 필드 출력
		void renderComponentField(const entt::meta_data& data, entt::meta_any& instance, bool& isModified);
		void renderEnumField(const entt::meta_data& data, entt::meta_any& instance, bool& isModified);
		void renderStructField(const entt::meta_data& data, entt::meta_any& instance, bool& isModified);

		// 컴포넌트 렌더 특수화
		void renderEntity(const entt::meta_data& data, entt::meta_any& instance, bool& isModified);
		void renderInterpolation(const entt::meta_data& data, entt::meta_any& instance, bool& isModified);
		void renderConstraints(const entt::meta_data& data, entt::meta_any& instance, bool& isModified);
		void renderMaterials(const entt::meta_data& data, entt::meta_any& instance, bool& isModified);
		void renderCookingOptions(const entt::meta_data& data, entt::meta_any& instance, bool& isModified);
		void renderAxis(const entt::meta_data& data, entt::meta_any& instance, bool& isModified);
		void renderButtonEvents(const entt::meta_data& data, entt::meta_any& instance, bool& isModified);

		/// \brief 특정 ctx 내의 타입들을 콤보 박스로 렌더링
		/// \param ctx 같은 계열의 메타 데이터가 저장된 context
		/// \param valueAny 저장되어 있는 값
		/// \param data 저장된 값의 타입 정보(메타 데이터)
		/// \param instance 저장된 값을 소유한 객체
		/// \param isModified 값 수정 여부
		void renderStructCombo(entt::meta_ctx& ctx, entt::meta_any& valueAny, const entt::meta_data& data, entt::meta_any& instance, bool& isModified);

		/// \brief customEnumCtx 내의 enum value 들을 콤보 박스로 렌더링
		/// \tparam T 출력하려는 enum 타입
		/// \param valueAny 저장되어 있는 값
		/// \param data 저장된 값의 타입 정보(메타 데이터)
		/// \param instance 저장된 값을 소유한 객체
		/// \param isModified 값 수정 여부
		template <typename T>
		void renderEnumCombo(entt::meta_any& valueAny, const entt::meta_data& data, entt::meta_any& instance, bool& isModified);

		/// \brief unordered_map<T, U> 를 콤보 박스로 렌더링
		/// \tparam T const char* 을 반환하는 c_str() 함수를 가진 타입
		/// \tparam U 사용자 임의 타입
		/// \param label 프로퍼티 이름
		/// \param currentItem 미리보기로 출력할 이름
		/// \param items <T, U> 타입의 unordered_map
		/// \param data 저장된 값의 타입 정보(메타 데이터)
		/// \param instance 저장된 값을 소유한 객체
		/// \param isModified 값 수정 여부
		template <typename T> requires HasCStr<T>
		void renderStringCombo2(const char* label, T& currentItem, const auto& items, const entt::meta_data& data, entt::meta_any& instance, bool& isModified);

		template <typename T, typename U> requires HasCStr<T>
		void renderStringCombo(const char* label, T& currentItem, const std::unordered_map<T, U>& items, const entt::meta_data& data, entt::meta_any& instance, bool& isModified);

		template <typename T, typename U> requires HasCStr<T>
		void renderStringCombo(const char* label, T& currentItem, const std::map<T, U>& items, const entt::meta_data& data, entt::meta_any& instance, bool& isModified);

		// renderStringCombo 의 vector 버전. unordered_map 부분 참조
		template <typename T> requires HasCStr<T>
		void renderStringCombo(const char* label, T& currentItem, std::vector<T>& items, const entt::meta_data& data, entt::meta_any& instance, bool& isModified);

		// renderStringCombo 의 vector 버전. unordered_map 부분 참조
		template <typename T> requires HasCStr<T>
		void renderStringCombo(const char* label, T& currentItem, const char* path, const char* extension, const entt::meta_data& data, entt::meta_any& instance, bool& isModified);

		/// \brief enum 을 플래그 형식으로 다중 선택 가능한 체크 박스로 렌더링
		/// \tparam T uint32_t 로 캐스팅 가능한 enum 타입
		/// \param valueAny 저장되어 있는 값
		/// \param data 저장된 값의 타입 정보(메타 데이터)
		/// \param instance 저장된 값을 소유한 객체
		/// \param isModified 값 수정 여부
		template <typename T>
		void renderEnumFlag(entt::meta_any& valueAny, const entt::meta_data& data, entt::meta_any& instance, bool& isModified);

		template <typename T>
		void renderSequenceContainer(const entt::meta_data& data, entt::meta_any& instance, bool& isModified);

		// 이벤트
		void selectEntity(const OnToolSelectEntity& event);
		void removeComponent(OnToolRemoveComponent& event);

	private:
		std::vector<entt::entity> _selectedEntities;
		entt::meta_any _copiedComponent;

		Renderer* _renderer = nullptr;
	};

	template <typename T>
	void ComponentDataPanel::renderEnumCombo(entt::meta_any& valueAny, const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
	{
		const auto dataName = data.prop("name"_hs).value().cast<const char*>();
		const auto dataType = entt::resolve(global::customEnumMetaCtx, data.type().info());
		auto val = valueAny.cast<T>();

		std::vector<std::tuple<T, const char*, const char*>> names;
		const char* currentName = nullptr;


		for (auto&& [enumId, enumData] : dataType.data())
		{
			T enumValue = enumData.prop("value"_hs).value().cast<T>();
			auto name = enumData.prop("name"_hs).value().cast<const char*>();

			if (enumData.prop("description"_hs))
			{
				auto desc = enumData.prop("description"_hs).value().cast<const char*>();
				names.emplace_back(enumValue, name, desc);
			}
			else
			{
				names.emplace_back(enumValue, name, nullptr);
			}

			if (enumValue == val)
			{
				currentName = name;
			}
		}

		if (ImGui::BeginCombo(dataName, currentName))
		{
			for (const auto& [enumValue, name, desc] : names)
			{
				bool isSelected = (enumValue == val);

				if (ImGui::Selectable(name, isSelected))
				{
					val = enumValue;
					data.set(entt::meta_handle{ instance }, val);
					isModified = true;
				}
				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
				if (ImGui::IsItemHovered() && desc)
				{
					ImGui::SetTooltip(desc);
				}
			}
			ImGui::EndCombo();
		}
	}

	template <typename T, typename U> requires HasCStr<T>
	void ComponentDataPanel::renderStringCombo(const char* label, T& currentItem, const std::unordered_map<T, U>& items, const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
	{
		const char* currentStr = currentItem.c_str();

		if (ImGui::BeginCombo(label, currentStr))
		{
			for (auto&& [key, value] : items)
			{
				bool isSelected = (currentItem == key);
				if (ImGui::Selectable(key.c_str(), isSelected))
				{
					currentItem = key;
					data.set(entt::meta_handle{ instance }, currentItem);
					isModified = true;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	template <typename T> requires HasCStr<T>
	void ComponentDataPanel::renderStringCombo2(const char* label, T& currentItem, const auto& items, const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
	{
		const char* currentStr = currentItem.c_str();

		if (ImGui::BeginCombo(label, currentStr))
		{
			for (auto&& [key, value] : items)
			{
				bool isSelected = (currentItem == key);
				if (ImGui::Selectable(key.c_str(), isSelected))
				{
					currentItem = key;
					data.set(entt::meta_handle{ instance }, currentItem);
					isModified = true;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	template <typename T> requires HasCStr<T>
	void ComponentDataPanel::renderStringCombo(const char* label, T& currentItem, std::vector<T>& items,
		const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
	{
		const char* currentStr = currentItem.c_str();

		if (ImGui::BeginCombo(label, currentStr))
		{
			for (auto&& item : items)
			{
				bool isSelected = (currentItem == item);
				if (ImGui::Selectable(item.c_str(), isSelected))
				{
					currentItem = item;
					data.set(entt::meta_handle{ instance }, currentItem);
					isModified = true;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	template <typename T> requires HasCStr<T>
	void ComponentDataPanel::renderStringCombo(const char* label, T& currentItem, const char* path,
		const char* extension, const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
	{
		const char* currentStr = currentItem.c_str();

		if (ImGui::BeginCombo(label, currentStr))
		{
			auto&& items = getFilesWithExtension(path, extension, true);
			for (auto&& item : items)
			{
				bool isSelected = (currentItem == item);
				if (ImGui::Selectable(item.c_str(), isSelected))
				{
					currentItem = item;
					data.set(entt::meta_handle{ instance }, currentItem);
					isModified = true;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	template <typename T, typename U> requires HasCStr<T>
	void ComponentDataPanel::renderStringCombo(const char* label, T& currentItem, const std::map<T, U>& items, const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
	{
		const char* currentStr = currentItem.c_str();

		if (ImGui::BeginCombo(label, currentStr))
		{
			for (auto&& [key, value] : items)
			{
				bool isSelected = (currentItem == key);
				if (ImGui::Selectable(key.c_str(), isSelected))
				{
					currentItem = key;
					data.set(entt::meta_handle{ instance }, currentItem);
					isModified = true;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	template <typename T>
	void ComponentDataPanel::renderEnumFlag(entt::meta_any& valueAny, const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
	{
		const auto dataName = data.prop("name"_hs).value().cast<const char*>();
		const auto dataType = entt::resolve(global::customEnumMetaCtx, data.type().info());
		auto val = static_cast<uint32_t>(valueAny.cast<T>());

		std::vector<std::tuple<T, const char*, const char*>> names;

		for (auto&& [enumId, enumData] : dataType.data())
		{
			T enumValue = enumData.prop("value"_hs).value().cast<T>();
			auto name = enumData.prop("name"_hs).value().cast<const char*>();

			if (enumData.prop("description"_hs))
			{
				auto desc = enumData.prop("description"_hs).value().cast<const char*>();
				names.emplace_back(enumValue, name, desc);
			}
			else
			{
				names.emplace_back(enumValue, name, nullptr);
			}
		}

		if (ImGui::TreeNode(dataName))
		{
			for (const auto& [enumValue, name, desc] : names)
			{
				bool isSelected = (val & enumValue) == enumValue;

				if (ImGui::Checkbox(name, &isSelected))
				{
					if (isSelected)
					{
						val |= static_cast<uint32_t>(enumValue); // 플래그 추가
					}
					else
					{
						val &= ~static_cast<uint32_t>(enumValue); // 플래그 제거
					}
					data.set(entt::meta_handle{ instance }, static_cast<T>(val));
					isModified = true;
				}

				if (ImGui::IsItemHovered() && desc)
				{
					ImGui::SetTooltip("%s", desc);
				}
			}
			ImGui::TreePop();
		}
	}

	template <typename T>
	void ComponentDataPanel::renderSequenceContainer(const entt::meta_data& data, entt::meta_any& instance, bool& isModified)
	{
		// 시퀀스 컨테이너인지 확인
		auto valueAny = data.get(entt::meta_handle{ instance });

		if (valueAny.type().info().hash() == entt::type_hash<T>().value())
		{
			auto& container = valueAny.cast<T&>();

			// 컨테이너의 이름 또는 필드명 출력
			if (ImGui::TreeNode(data.prop("name"_hs).value().cast<const char*>()))
			{
				int index = 0;

				// 컨테이너의 각 요소를 렌더링
				for (auto& element : container)
				{
					ImGui::PushID(index);
					if (ImGui::TreeNode(("Element " + std::to_string(index)).c_str()))
					{
						// 요소를 메타 데이터를 통해 재귀적으로 렌더링
						entt::meta_any elementMeta = entt::meta_any(element);
						bool elementModified = false;

						//if (elementMeta.type().id() == entt::type_hash<std::string>())
						//{
						//	auto val = elementMeta.cast<std::string>();
						//	//const auto propName = data.prop("name"_hs).value().cast<const char*>();

						//	if (ImGui::InputText("text", &val))
						//	{
						//		data.set(entt::meta_handle{ elementMeta }, val);
						//		elementModified = true;
						//	}
						//}
						//else
						//{
						//}
							for (auto&& [id, field] : entt::resolve(global::customStructMetaCtx, elementMeta.type().id()).data())
							{
								renderComponentField(field, elementMeta, elementModified);
							}

						if (elementModified)
						{
							container[index] = elementMeta.cast<typename T::value_type>();
							isModified = true;
						}

						ImGui::TreePop();
					}
					ImGui::PopID();
					++index;
				}

				// 요소 추가 및 제거 버튼
				if (ImGui::Button(" + "))
				{
					container.emplace_back(); // 기본 생성자로 새 요소 추가
					isModified = true;
				}
				if (!container.empty() && ImGui::Button(" - "))
				{
					container.pop_back(); // 마지막 요소 제거
					isModified = true;
				}

				if (isModified)
					data.set(entt::meta_handle{ instance }, container);

				ImGui::TreePop();
			}
		}
	}
}
