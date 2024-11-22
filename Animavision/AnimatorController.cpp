#include "pch.h"
#include "AnimatorController.h"
#include "AnimationHelper.h"
#include "ModelLoader.h"


void AnimatorController::Initialize(Model* model)
{
	if (m_Bones.size() > 0 && m_CurrentAnimationClip != nullptr)
		return;

	auto& animations = model->GetAnimations();
	auto& nodes = model->GetNodes();
	auto& bones = model->GetBones();

	std::vector<Matrix> toRoot;

	m_Bones.reserve(bones.size());
	for (auto& curBone : bones)
	{
		Bone bone;
		bone.name = curBone->name;
		bone.offsetMatrix = curBone->offsetMatrix;

		m_Bones.push_back(std::move(bone));
	}

	m_FinalTransforms.resize(m_Bones.size());
	for (auto& animation : animations)
		m_AnimationMap[animation->name] = animation;

	for (auto& node : nodes)
	{
		m_NodeMap[node->name] = node;
	}
}

void AnimatorController::Finalize()
{
	for (auto& animation : m_AnimationMap)
	{
		if (animation.second != nullptr)
			delete animation.second;
	}

	m_AnimationMap.clear();
	m_NodeMap.clear();
}

void AnimatorController::Update(float deltaTime)
{
	if (m_CurrentAnimationClip == nullptr || !m_CurrentAnimationClip->isPlay)
		return;

	m_CurrentAnimationClip->currentTimePos += deltaTime;
	if (m_CurrentAnimationClip->currentTimePos > m_CurrentAnimationClip->duration)
	{
		if (m_CurrentAnimationClip->isLoop)
			m_CurrentAnimationClip->currentTimePos = fmod(m_CurrentAnimationClip->currentTimePos, m_CurrentAnimationClip->duration);
		else
			m_CurrentAnimationClip->isPlay = false;
	}

	checkTransitions();

	if (m_BlendTime > 0.0f)
	{
		blendAnimations(deltaTime);
		return;
	}

	auto& nodeclips = m_CurrentAnimationClip->nodeClips;

	for (auto& transform : m_FinalTransforms)
		transform = Matrix::Identity;

	// 로컬 트랜스폼 계산
	for (int i = 0; i < m_Bones.size(); i++)
	{
		std::string curBoneName = m_Bones[i].name;

		for (auto& nodeclip : nodeclips)
		{
			auto& keyframes = nodeclip->keyframes;
			auto findIt = m_NodeMap.find(curBoneName);

			if (findIt != m_NodeMap.end())
			{
				auto matrix = AnimationHelper::ComputeAnimationTransform(*m_CurrentAnimationClip, curBoneName, m_CurrentAnimationClip->currentTimePos);

				auto node = findIt->second;
				node->nodeTransform = matrix;
			}
		}
	}

	std::vector<Matrix> toRoots(m_NodeMap.size());
	std::map<int, Node*> tempNodeMap;

	for (const auto& [name, node] : m_NodeMap)
	{
		tempNodeMap.insert({ node->index, node });
	}

	auto find = tempNodeMap.find(0);
	assert(find != tempNodeMap.end());

	toRoots[0] = find->second->nodeTransform;
	for (size_t i = 1; i < m_NodeMap.size(); ++i)
	{
		find = tempNodeMap.find(static_cast<const int>(i));
		assert(find != tempNodeMap.end());

		toRoots[i] = find->second->nodeTransform * toRoots[find->second->parentIndex];
	}

	for (size_t i = 0; i < m_Bones.size(); ++i)
	{
		auto& bone = m_Bones[i];
		auto findedNode = m_NodeMap.find(bone.name);
		assert(findedNode != m_NodeMap.end());
		m_FinalTransforms[i] = (bone.offsetMatrix * toRoots[findedNode->second->index]).Transpose();
	}
	// 매쉬에 저장된 aiBone은 현재 메쉬가 참조할 수 있는 가능성이 있는 노드구나 ->
	// 노드 계층구조가 유효하게 유지될라면 노드 순회를 타야한다.
	// 지금 Bone


	for (auto& trigger : m_TriggerParameters)
		trigger.second = false;
}

void AnimatorController::SetDefaultAnimation(const std::string& name)
{
	auto findIt = m_AnimationMap.find(name);

	if (findIt != m_AnimationMap.end())
	{
		m_CurrentAnimationClip = const_cast<AnimationClip*>(findIt->second);
		m_CurrentAnimationClip->currentTimePos = 0.0f;
	}
	else
	{
		m_CurrentAnimationClip = nullptr;
		OutputDebugStringA("Animation not found\n");
	}
}

void AnimatorController::SetAnimation(const std::string& name, float blendTime /*= 0.0f*/)
{
	if (findAnimationClip(name))
	{
		m_NextClip = m_FoundClip;
		m_BlendTime = blendTime;
		m_BlendProgress = 0.0f;
	}
}

bool AnimatorController::AddAnimation(const AnimationClip* clip)
{
	if (clip == nullptr)
		return false;

	m_AnimationMap[clip->name] = clip;
	return true;
}

bool AnimatorController::AddTransition(const std::string& from, const std::string& to, float blendTime, const std::vector<AnimationCondition>& conditions)
{
	// if no Animation
	if (m_AnimationMap.find(from) == m_AnimationMap.end() || m_AnimationMap.find(to) == m_AnimationMap.end())
		return false;

	m_Transitions[from].emplace_back(to, blendTime, conditions);
	return true;
}

void AnimatorController::AddBoolParameter(const std::string& name, bool value)
{
	m_BoolParameters[name] = value;
}

void AnimatorController::AddFloatParameter(const std::string& name, float value)
{
	m_FloatParameters[name] = value;
}

void AnimatorController::AddTriggerParameter(const std::string& name)
{

}

bool AnimatorController::GetBoolParameter(const std::string& name) const
{
	auto& findIt = m_BoolParameters.find(name);

	if (findIt != m_BoolParameters.end())
		return findIt->second;

	else
		OutputDebugStringA("Bool Parameter not found\n");
	return false;
}

float AnimatorController::GetFloatParameter(const std::string& name) const
{
	auto& findIt = m_FloatParameters.find(name);

	if (findIt != m_FloatParameters.end())
		return findIt->second;

	else
		OutputDebugStringA("Float Parameter not found\n");

	return 0.0f;
}

bool AnimatorController::GetTriggerParameter(const std::string& name) const
{
	return false;
}

void AnimatorController::SetBoolParameter(const std::string& name, bool value)
{
	auto& findIt = m_BoolParameters.find(name);

	if (findIt != m_BoolParameters.end())
		findIt->second = value;

	else
		OutputDebugStringA("Bool Parameter not found\n");
}

void AnimatorController::SetFloatParameter(const std::string& name, float value)
{
	auto& findIt = m_FloatParameters.find(name);

	if (findIt != m_FloatParameters.end())
		findIt->second = value;

	else
		OutputDebugStringA("Float Parameter not found\n");
}

void AnimatorController::SetTriggerParameter(const std::string& name)
{

}

void AnimatorController::EraseBoolParameter(const std::string& name)
{
	auto& findIt = m_BoolParameters.find(name);

	if (findIt != m_BoolParameters.end())
		m_BoolParameters.erase(findIt);
	else
		OutputDebugStringA("Bool Parameter not found\n");
}

void AnimatorController::EraseFloatParameter(const std::string& name)
{
	auto& findIt = m_FloatParameters.find(name);

	if (findIt != m_FloatParameters.end())
		m_FloatParameters.erase(findIt);
	else
		OutputDebugStringA("Float Parameter not found\n");
}

void AnimatorController::EraseTriggerParameter(const std::string& name)
{

}

bool AnimatorController::findAnimationClip(const std::string& name)
{
	auto findIt = m_AnimationMap.find(name);

	if (findIt != m_AnimationMap.end())
		m_FoundClip = const_cast<AnimationClip*>(findIt->second);
	else
		return false;

	return true;
}

void AnimatorController::blendAnimations(float deltaTime)
{
	if (m_NextClip == nullptr || m_BlendTime <= 0.0f)
		return;

	m_BlendProgress += deltaTime / m_BlendTime;
	if (m_BlendProgress >= 1.0f)
	{
		m_CurrentAnimationClip = m_NextClip;
		m_NextClip = nullptr;
		m_BlendProgress = 0.0f;
		m_BlendTime = 0.0f;
		return;
	}

	// 로컬 트랜스폼 계산
	for (int i = 0; i < m_Bones.size(); i++)
	{
		std::string curBoneName = m_Bones[i].name;

		for (auto& nodeclip : m_CurrentAnimationClip->nodeClips)
		{
			auto& keyframes = nodeclip->keyframes;
			auto findIt = m_NodeMap.find(curBoneName);

			if (findIt != m_NodeMap.end())
			{
				auto matrix = AnimationHelper::BlendTransforms(*m_CurrentAnimationClip, *m_NextClip, curBoneName, m_BlendProgress);

				auto node = findIt->second;
				node->nodeTransform = matrix;
			}
		}
	}

	std::vector<Matrix> toRoots(m_NodeMap.size());
	std::map<int, Node*> tempNodeMap;

	for (const auto& [name, node] : m_NodeMap)
	{
		tempNodeMap.insert({ node->index, node });
	}

	auto find = tempNodeMap.find(0);
	assert(find != tempNodeMap.end());

	toRoots[0] = find->second->nodeTransform;
	for (size_t i = 1; i < m_NodeMap.size(); ++i)
	{
		find = tempNodeMap.find(static_cast<const int>(i));
		assert(find != tempNodeMap.end());

		toRoots[i] = find->second->nodeTransform * toRoots[find->second->parentIndex];
	}

	for (size_t i = 0; i < m_Bones.size(); ++i)
	{
		auto& bone = m_Bones[i];
		auto findedNode = m_NodeMap.find(bone.name);
		assert(findedNode != m_NodeMap.end());
		m_FinalTransforms[i] = (bone.offsetMatrix * toRoots[findedNode->second->index]).Transpose();
	}
}

void AnimatorController::checkTransitions()
{
	if (!m_CurrentAnimationClip)
		return;

	const auto& transitions = m_Transitions[m_CurrentAnimationClip->name];
	for (const auto& transition : transitions)
	{
		if (transition.CheckConditions(m_BoolParameters, m_FloatParameters, m_TriggerParameters))
		{
			SetAnimation(transition.GetTargetAnimation(), transition.GetBlendTime());
			break;
		}
	}
}
