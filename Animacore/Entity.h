#pragma once

namespace core
{
	class Entity
	{
	public:
		Entity(entt::entity handle, entt::registry& registry) : _handle(handle), _registry(registry) {}

		template <typename T, typename... Args>
		T& Emplace(Args&&... args);

		template <typename T, typename... Args>
		T& GetOrEmplace(Args&&... args);

		template <typename T, typename... Args>
		T& Replace(Args&&... args);

		template <typename T>
		T& Get() const;

		template <typename T>
		T* TryGet() const;

		template<typename... Args>
		bool HasAllOf() const;

		template<typename... Args>
		bool HasAnyOf() const;

		template<typename... Args>
		void Remove() const;

		void Destroy() const { _registry.destroy(_handle); }

		/// \brief 인자를 부모로 설정
		/// \param entity 부모로 설정하려는 엔티티, entt::null 일 경우 자신을 최상위로 변경
		void SetParent(entt::entity entity);

		std::vector<entt::entity> GetChildren() const;

		/// \brief 현재 엔티티가 인자의 상위 계층에 속해있는지 검사
		/// \param entity 검사할 엔티티
		/// \return 상위 계층에 속해 있을 경우 true, 이외 false
		bool IsAncestorOf(entt::entity entity) const;

		/// \brief 현재 엔티티가 인자의 하위 계층에 속해있는지 검사
		/// \param entity 검사할 엔티티
		/// \return 하위 계층에 속해 있을 경우 true, 이외 false
		bool IsDescendantOf(entt::entity entity) const;

		const entt::entity& GetHandle() const { return _handle; }

		operator uint32_t () const { return static_cast<uint32_t>(_handle); }
		operator entt::entity() const { return _handle; }
		operator bool() const { return (_handle != entt::null) && _registry.valid(_handle); }

		Entity& operator=(const Entity& other) { if (this == &other) { return *this; } _handle = other._handle; return *this; }
		bool operator==(const Entity& other) const { return _handle == other._handle && &_registry == &other._registry; }
		bool operator!=(const Entity& other) const { return !(*this == other); }

	private:
		entt::entity _handle = entt::null;
		entt::registry& _registry;
	};

	template <typename T, typename... Args>
	T& Entity::Emplace(Args&&... args)
	{
		return _registry.emplace<T>(_handle, std::forward<Args>(args)...);
	}

	template <typename T, typename ... Args>
	T& Entity::GetOrEmplace(Args&&... args)
	{
		return _registry.get_or_emplace<T>(_handle, std::forward<Args>(args)...);
	}

	template <typename T, typename ... Args>
	T& Entity::Replace(Args&&... args)
	{
		return _registry.replace<T>(_handle, std::forward<Args>(args)...);
	}

	template <typename T>
	T& Entity::Get() const
	{
		return _registry.get<T>(_handle);
	}

	template <typename T>
	T* Entity::TryGet() const
	{
		return _registry.try_get<T>(_handle);
	}

	template <typename... Args>
	bool Entity::HasAllOf() const
	{
		return _registry.all_of<Args...>(_handle);
	}

	template <typename ... Args>
	bool Entity::HasAnyOf() const
	{
		return _registry.any_of<Args...>(_handle);
	}

	template <typename... Args>
	void Entity::Remove() const
	{
		_registry.remove<Args...>(_handle);
	}
}


