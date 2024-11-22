#pragma once

#include "Mesh.h"
#include "RendererDLL.h"

#include <map>

class Model;
struct Bone;
class AnimationTransition;
struct AnimationCondition;

class ANIMAVISION_DLL AnimatorController
{
public:
	enum class StateType
	{
		ENTRY,
		EXIT,
		STATE,
		ANYSTATE,
	};

	AnimatorController() = default;
	~AnimatorController() = default;

	void Initialize(Model* model);
	void Finalize();

	void Update(float deltaTime);
	void SetDefaultAnimation(const std::string& name);
	void SetAnimation(const std::string& name, float blendTime = 0.0f);
	bool AddAnimation(const AnimationClip* clip);
	bool AddTransition(const std::string& from, const std::string& to, float blendTime, const std::vector<AnimationCondition>& conditions);
	void AddBoolParameter(const std::string& name, bool value);
	void AddFloatParameter(const std::string& name, float value);
	void AddTriggerParameter(const std::string& name);
	bool GetBoolParameter(const std::string& name) const;
	float GetFloatParameter(const std::string& name) const;
	bool GetTriggerParameter(const std::string& name) const;
	void SetBoolParameter(const std::string& name, bool value);
	void SetFloatParameter(const std::string& name, float value);
	void SetTriggerParameter(const std::string& name);
	void EraseBoolParameter(const std::string& name);
	void EraseFloatParameter(const std::string& name);
	void EraseTriggerParameter(const std::string& name);

	inline void Play() { m_CurrentAnimationClip->isPlay = true; }
	inline void Stop() { m_CurrentAnimationClip->isPlay = false; m_CurrentAnimationClip->currentTimePos = 0.0f; }
	inline void Pause() { m_CurrentAnimationClip->isPlay = false; }

	inline const std::vector<Matrix> GetFinalTransforms() const { return m_FinalTransforms; }
	inline std::map<std::string, bool>& GetBoolParameters() { return m_BoolParameters; }
	inline std::map<std::string, float>& GetFloatParameters() { return m_FloatParameters; }
	inline std::map<std::string, bool>& GetTriggerParameters() { return m_TriggerParameters; }

private:
	inline bool findAnimationClip(const std::string& name);
	void blendAnimations(float deltaTime);
	void checkTransitions();

private:
	AnimationClip* m_CurrentAnimationClip = nullptr;
	AnimationClip* m_FoundClip = nullptr;
	AnimationClip* m_NextClip = nullptr;
	float m_BlendTime = 0.0f;
	float m_BlendProgress = 0.0f;
	std::map<std::string, const AnimationClip*> m_AnimationMap;
	std::map<std::string, Node*> m_NodeMap;
	std::map<std::string, std::vector<AnimationTransition>> m_Transitions;
	std::vector<Bone> m_Bones;
	std::vector<Matrix> m_FinalTransforms;

	std::map<std::string, bool> m_BoolParameters;
	std::map<std::string, float> m_FloatParameters;
	std::map<std::string, bool> m_TriggerParameters;
};

enum class ConditionType
{
	BOOL,
	FLOAT,
	TRIGGER
};

struct AnimationCondition
{
	ConditionType type;
	std::string parameterName;
	bool boolValue;
	float floatValue;
	float threshold;
	bool greaterThan;

	bool Evaluate(const std::map<std::string, bool>& boolParams,
		const std::map<std::string, float>& floatParams,
		const std::map<std::string, bool>& triggerParams) const
	{
		switch (type)
		{
			case ConditionType::BOOL:
				return boolParams.at(parameterName) == boolValue;
			case ConditionType::FLOAT:
				return greaterThan ? floatParams.at(parameterName) > threshold : floatParams.at(parameterName) < threshold;
			case ConditionType::TRIGGER:
				return triggerParams.at(parameterName);
			default:
				return false;
		}
	}
};

class AnimationTransition
{
public:
	AnimationTransition(const std::string& targetAnimation, float blendTime, const std::vector<AnimationCondition>& conditions)
		: m_TargetAnimation(targetAnimation), m_BlendTime(blendTime), m_Conditions(conditions) {}

	const std::string& GetTargetAnimation() const { return m_TargetAnimation; }
	float GetBlendTime() const { return m_BlendTime; }

	bool CheckConditions(const std::map<std::string, bool>& boolParams,
		const std::map<std::string, float>& floatParams,
		const std::map<std::string, bool>& triggerParams) const
	{
		for (const auto& condition : m_Conditions)
		{
			if (!condition.Evaluate(boolParams, floatParams, triggerParams))
				return false;
		}

		return true;
	}

private:
	std::string m_TargetAnimation;
	float m_BlendTime;
	std::vector<AnimationCondition> m_Conditions;
};