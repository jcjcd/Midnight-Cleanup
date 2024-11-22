#include "pch.h"
#include "Entity.h"
#include "CoreComponents.h"

void core::Entity::SetParent(entt::entity entity)
{
    auto parent = Entity{ entity, _registry };
    if (parent._handle == entt::null)
    {
        if (HasAnyOf<Relationship>())
        {
            Relationship& relationship = Get<Relationship>();
            if (relationship.parent != entt::null)
            {
                auto& oldParentRelationship = _registry.get<Relationship>(relationship.parent);
                std::erase(oldParentRelationship.children, *this);
                relationship.parent = entt::null;
            }
        }
        return;
    }

    // 부모가 Relationship을 가지고 있지 않다면 추가
    if (!parent.HasAnyOf<Relationship>())
    {
        parent.Emplace<core::Relationship>();
    }

    // 현재 엔티티가 Relationship을 가지고 있지 않다면 추가
    if (!HasAnyOf<Relationship>())
    {
        Relationship& relationship = Emplace<Relationship>();
        relationship.parent = entity;
    }
    else
    {
        Relationship& relationship = Get<Relationship>();

        // 부모가 변경된 경우 기존 부모의 자식 목록에서 제거
        if (relationship.parent != entity)
        {
            if (relationship.parent != entt::null)
            {
                auto& oldParentRelationship = _registry.get<Relationship>(relationship.parent);
                std::erase(oldParentRelationship.children, *this);
            }

            // 부모 설정 변경
            relationship.parent = entity;
        }
    }

    // 새로운 부모의 children에 추가 (부모가 동일한 경우에도 다시 추가)
    auto& parentChildren = parent.Get<Relationship>().children;

    // 자식 목록에 중복으로 추가되지 않도록 확인 후 추가
    if (std::ranges::find(parentChildren, *this) == parentChildren.end())
        parentChildren.push_back(*this);
}

std::vector<entt::entity> core::Entity::GetChildren() const
{
	if (HasAnyOf<Relationship>())
	{
		const auto& relationship = Get<Relationship>();
		return relationship.children;
	}
	return {};
}


bool core::Entity::IsAncestorOf(entt::entity entity) const
{
	while (_registry.any_of<Relationship>(entity))
	{
		const auto& relationship = _registry.get<Relationship>(entity);

		if (relationship.parent == *this)
			return true;

		entity = Entity(relationship.parent, _registry);
	}

	return false;
}

bool core::Entity::IsDescendantOf(entt::entity entity) const
{
	auto children = Entity(entity, _registry).GetChildren();

	while (!children.empty())
	{
		std::vector<entt::entity> descendants;

		for (const auto& child : children)
		{
			if (child == this->_handle)
				return true;

			if (const auto rel = _registry.try_get<Relationship>(child))
				descendants.insert(descendants.end(), rel->children.begin(), rel->children.end());
		}

		children = std::move(descendants);
	}

	return false;
}