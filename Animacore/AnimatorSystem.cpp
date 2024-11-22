#include "pch.h"
#include "AnimatorSystem.h"

#include "Scene.h"
#include "CoreComponents.h"
#include "RenderComponents.h"
#include "CoreSystemEvents.h"

#include "AnimatorState.h"

#include "../Animavision/Mesh.h"

#include "../Animavision/Renderer.h"
#include "../Animavision/AnimationHelper.h"

#include "MetaCtxs.h"

// test
core::AnimatorSystem::AnimatorSystem(Scene& scene)
	: ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<OnCreateEntity>().connect<&AnimatorSystem::createEntity>(this);
	_dispatcher->sink<OnStartSystem>().connect<&AnimatorSystem::startSystem>(this);
	_dispatcher->sink<OnFinishSystem>().connect<&AnimatorSystem::finishSystem>(this);
}


bool core::AnimatorSystem::processTransition(core::Animator& animator, core::AnimatorTransition& transition)
{
	// 현재 스테이트와 목적 스테이트가 같으면 무시
	if (animator._currentState == &animator._states->at(transition.destinationState))
	{
		return false;
	}

	bool isTransition = true;
	for (auto& condition : transition.conditions)
	{
		if (condition.threshold.type() == entt::resolve<int>())
		{
			int lhs = animator.parameters.at(condition.parameter).value.cast<int>();
			int rhs = condition.threshold.cast<int>();

			isTransition = compare(lhs, rhs, condition.mode);
		}
		else if (condition.threshold.type() == entt::resolve<float>())
		{
			float lhs = animator.parameters.at(condition.parameter).value.cast<float>();
			float rhs = condition.threshold.cast<float>();

			isTransition = compare(lhs, rhs, condition.mode);
		}
		else if (condition.threshold.type() == entt::resolve<bool>())
		{
			bool lhs = animator.parameters.at(condition.parameter).value.cast<bool>();
			bool rhs = condition.threshold.cast<bool>();

			if (condition.mode == AnimatorCondition::Mode::Trigger)
			{
				isTransition = lhs;
				if (isTransition)
				{
					animator.parameters.at(condition.parameter).value = false;
				}
			}
			else
			{
				isTransition = compare(lhs, rhs, condition.mode);
			}
		}
		else if (condition.threshold.type() == entt::resolve<std::string>())
		{
			std::string lhs = animator.parameters.at(condition.parameter).value.cast<std::string>();
			std::string rhs = condition.threshold.cast<std::string>();

			isTransition = compare(lhs, rhs, condition.mode);
		}
		if (isTransition == false)
			break;
	}
	if (isTransition)
	{
		animator._nextState = &animator._states->at(transition.destinationState);
		if (animator._nextState->motion)
		{
			auto it = animator._nodeClips.find(transition.destinationState);
			if (it != animator._nodeClips.end())
				animator._nextNodeClips = &it->second;
			else
				animator._nextNodeClips = nullptr;

			animator._nextDuration = animator._nextState->motion->duration;
		}
		animator._currentTransition = &transition;
		return true;
	}

	return false;
}

void core::AnimatorSystem::operator()(Scene& scene, Renderer& renderer, float tick)
{
	if(!scene.IsPlaying())
		return;

	auto&& registry = scene.GetRegistry();
	auto view = registry->view<Animator>();

	auto meta = entt::resolve<global::CallBackFuncDummy>(global::callbackEventMetaCtx);

	for (auto&& [entity, animator] : view.each())
	{
		animator._currentTimePos += tick * animator._currentState->multiplier;
		if (animator._currentState)
		{
			// 애니메이션의 이벤트는 여기서 처리
			for (auto& event : animator._currentState->animationEvents)
			{
				if (!event.functionName.empty())
				{
					if (event.time < animator._currentTimePos / animator._currentDuration && !event.isProcessed)
					{
						event.parameters[0] = &scene;
						meta.func(entt::hashed_string{ event.functionName.c_str() }).invoke({}, event.parameters.data(), event.parameters.size());
						event.isProcessed = true;
					}
				}
			}

			if (animator._currentState->isLoop)
			{
				if (animator._currentTimePos > animator._currentDuration)
				{
					animator._currentTimePos = fmod(animator._currentTimePos, animator._currentDuration);
					for (auto& event : animator._currentState->animationEvents)
					{
						event.isProcessed = false;
					}
				}
			}
			else
			{
				if (animator._currentTimePos > animator._currentDuration)
				{
					animator._currentTimePos = animator._currentDuration;
				}
			}

			// check transition
			for (auto& transition : animator._currentState->transitions)
			{
				if (transition.exitTime > 0.0f)
				{
					if (animator._currentTimePos / animator._currentDuration < transition.exitTime)
					{
						continue;
					}
				}
				
				if (processTransition(animator, transition))
				{
					break;
				}
			}

			// 애니 스테이트의 트랜지션 검사
			if (animator._anyState)
			{
				for (auto& transition : animator._anyState->transitions)
				{
					if (transition.exitTime > 0.0f)
					{
						if (animator._currentTimePos / animator._currentDuration < transition.exitTime)
						{
							continue;
						}
					}

					if (processTransition(animator, transition))
					{
						break;
					}
				}
			}
		}

		if (animator._nextState)
		{
			if (animator._currentBlendTime < animator._currentTransition->blendTime)
			{
				animator._currentBlendTime += tick;
				animator._nextTimePos += tick * animator._nextState->multiplier;

				// 여기도 이벤트 처리
				for (auto& event : animator._nextState->animationEvents)
				{
					if (!event.functionName.empty())
					{
						if (event.time < animator._nextTimePos / animator._nextDuration && !event.isProcessed)
						{
							meta.func(entt::hashed_string{ event.functionName.c_str() }).invoke({}, event.parameters.data(), event.parameters.size());
							event.isProcessed = true;
						}
					}
				}

				if (animator._nextState->isLoop)
				{
					if (animator._nextTimePos > animator._nextDuration)
					{
						animator._nextTimePos = fmod(animator._nextTimePos, animator._nextDuration);
						for (auto& event : animator._nextState->animationEvents)
						{
							event.isProcessed = false;
						}
					}
				}
				else
				{
					if (animator._nextTimePos > animator._nextDuration)
					{
						animator._nextTimePos = animator._nextDuration;
					}
				}

				float blendFactor = animator._currentBlendTime / animator._currentTransition->blendTime;

				if (animator._currentNodeClips && animator._nextNodeClips)
				{
					for (int i = 0; i < animator._currentNodeClips->size(); i++)
					{
						if (!(*animator._currentNodeClips)[i].second || !(*animator._nextNodeClips)[i].second)
							continue;

						Vector3 translation = AnimationHelper::interpolateTranslation((*animator._currentNodeClips)[i].second->keyframes, animator._currentTimePos);
						Quaternion rotation = AnimationHelper::interpolateRotation((*animator._currentNodeClips)[i].second->keyframes, animator._currentTimePos);
						Vector3 scale = AnimationHelper::interpolateScale((*animator._currentNodeClips)[i].second->keyframes, animator._currentTimePos);

						Vector3 nextTranslation = AnimationHelper::interpolateTranslation((*animator._nextNodeClips)[i].second->keyframes, animator._nextTimePos);
						Quaternion nextRotation = AnimationHelper::interpolateRotation((*animator._nextNodeClips)[i].second->keyframes, animator._nextTimePos);
						Vector3 nextScale = AnimationHelper::interpolateScale((*animator._nextNodeClips)[i].second->keyframes, animator._nextTimePos);

						auto blendRotation = Quaternion::Slerp(rotation, nextRotation, blendFactor);
						blendRotation.Normalize();

						(*animator._currentNodeClips)[i].first->position = (translation * (1.0f - blendFactor) + nextTranslation * blendFactor);
						(*animator._currentNodeClips)[i].first->rotation = (blendRotation);
						(*animator._currentNodeClips)[i].first->scale = (scale * (1.0f - blendFactor) + nextScale * blendFactor);
						registry->patch<LocalTransform>(entity);
					}
				}
			}
			else
			{
				animator._currentBlendTime = 0.0f;
				animator._currentState = animator._nextState;
				animator._currentTransition = nullptr;
				animator._currentNodeClips = animator._nextNodeClips;
				animator._nextState = nullptr;
				animator._nextNodeClips = nullptr;
				animator._currentTimePos = animator._nextTimePos;
				animator._nextTimePos = 0.0f;
				animator._currentDuration = animator._nextDuration;
				animator._nextDuration = 0.0f;

				// 이번스테이트 이벤트 초기화
				for (auto& event : animator._currentState->animationEvents)
				{
					event.isProcessed = false;
				}

			}

			// 여기서 바로 종료
			return;
		}

		if (animator._currentNodeClips)
		{
			for (int i = 0; i < animator._currentNodeClips->size(); i++)
			{
				if (!(*animator._currentNodeClips)[i].second)
					continue;
				Vector3 translation = AnimationHelper::interpolateTranslation((*animator._currentNodeClips)[i].second->keyframes, animator._currentTimePos);
				Quaternion rotation = AnimationHelper::interpolateRotation((*animator._currentNodeClips)[i].second->keyframes, animator._currentTimePos);
				Vector3 scale = AnimationHelper::interpolateScale((*animator._currentNodeClips)[i].second->keyframes, animator._currentTimePos);

				(*animator._currentNodeClips)[i].first->position = translation;
				(*animator._currentNodeClips)[i].first->rotation = rotation;
				(*animator._currentNodeClips)[i].first->scale = scale;
				registry->patch<LocalTransform>(entity);
			}
		}
	}
}

void core::AnimatorSystem::createEntity(const OnCreateEntity& event)
{
	//initEntity(event.entity, *event.scene);
}

void core::AnimatorSystem::startSystem(const OnStartSystem& event)
{
	auto view = event.scene->GetRegistry()->view<Animator>();

	// 각 애니메이터 컴포넌트마다 name 을 기반으로 _states 을 할당

	for (auto&& [entity, animator] : view.each())
	{
		initEntity({ entity, *event.scene->GetRegistry() }, *event.scene, *event.renderer);
	}
}

void core::AnimatorSystem::finishSystem(const OnFinishSystem& event)
{

}

void core::AnimatorSystem::initEntity(core::Entity entity, Scene& scene, Renderer& renderer)
{
	if (!entity.HasAnyOf<Animator>())
		return;

	auto& animator = entity.Get<Animator>();

	AnimatorController* controller = controllerManager.GetController(animator.animatorFileName);

	if (controller)
	{
		animator._states = &controller->stateMap;
		animator.parameters = controller->parameters;
	}

	std::vector<core::Entity> entities;
	entities.push_back(entity);
	// BFS로 순회하면서 엔티티를 다 찾읍시다.
	uint32_t size = 1;
	for (uint32_t i = 0; i < size; i++)
	{
		auto children = entities[i].GetChildren();
		for (auto& child : children)
		{
			entities.push_back(core::Entity(child, *scene.GetRegistry()));
			size++;
		}
	}

	for (auto&& entity : entities)
	{
		auto& localTransform = entity.Get<core::LocalTransform>();
		auto& worldTransform = entity.Get<core::WorldTransform>();
		auto& name = entity.Get<core::Name>();

		animator._boneMap.emplace(name.name, std::make_pair(&localTransform, &worldTransform));


		// 변경
		if (auto* meshRenderer = entity.TryGet<MeshRenderer>())
		{
			meshRenderer->animator = &animator;
			animator._meshRenderers.push_back(meshRenderer);
		}
	}

	for (auto& meshRenderer : animator._meshRenderers)
	{
		if (!meshRenderer->mesh)
			meshRenderer->mesh = renderer.GetMesh(meshRenderer->meshString);

		if (meshRenderer->isSkinned && meshRenderer->mesh)
		{
			meshRenderer->bones.resize(meshRenderer->mesh->subMeshDescriptors.size());
			meshRenderer->boneOffsets.resize(meshRenderer->mesh->subMeshDescriptors.size());
			for (int i = 0; i < meshRenderer->mesh->subMeshDescriptors.size(); i++)
			{
				auto& subMeshDescriptor = meshRenderer->mesh->subMeshDescriptors[i];
				auto& boneIndexMap = subMeshDescriptor.boneIndexMap;
				// 본 트랜스폼 포인터를 순서대로 매핑해준다.
				meshRenderer->bones[i].resize(boneIndexMap.size());
				meshRenderer->boneOffsets[i].resize(boneIndexMap.size());

				for (auto& bone : boneIndexMap)
				{
					auto findIt = animator._boneMap.find(bone.first);
					if (findIt != animator._boneMap.end())
					{
						meshRenderer->bones[i][bone.second.first] = findIt->second.second;
						meshRenderer->boneOffsets[i][bone.second.first] = bone.second.second;
					}
				}
			}
		}
	}

	if (animator._meshRenderers.size())
	{
		for (auto& state : *animator._states)
		{
			state.second._animator = &animator;

			if (!state.second.motion)
			{
				continue;
			}

			// 딱 이부분만 수정하면 됨
			std::vector<std::pair<LocalTransform*, std::shared_ptr<NodeClip>>> nodeClips;
			nodeClips.resize(state.second.motion->nodeClips.size());

			for (auto& nodeClip : state.second.motion->nodeClips)
			{
				auto findedTransform = animator._boneMap.find(nodeClip->nodeName);
				if (findedTransform != animator._boneMap.end())
				{
					nodeClips.push_back({ findedTransform->second.first, nodeClip });
				}
			}

			animator._nodeClips.emplace(state.first, std::move(nodeClips));
		}
	}

	// 엔트리가 있다는 것을 보장해야한다.
	auto entry = animator._states->find("Entry");
	if (entry != animator._states->end())
	{
		animator._currentState = &entry->second;
	}
	else
	{
		animator._currentState = &animator._states->begin()->second;
	}

	// 애니 스테이트를 확인한다.
	auto anyState = animator._states->find("Any State");
	if (anyState != animator._states->end())
	{
		animator._anyState = &anyState->second;
	}
	else
	{
		animator._anyState = nullptr;
	}
}

void core::ControllerManager::LoadControllersFromDrive(const std::string& path, Renderer* renderer)
{
	std::filesystem::path controllerPath = path;

	if (!std::filesystem::exists(controllerPath))
	{
		return;
	}

	for (const auto& entry : std::filesystem::recursive_directory_iterator(controllerPath))
	{
		if (entry.is_regular_file())
		{
			if (entry.path().extension() == ".controller")
			{
				LoadController(entry.path().string(), renderer);
			}
		}
	}

}

// 이때 스테이트에 있는 모션도 매핑해야한다.
core::AnimatorController* core::ControllerManager::LoadController(const std::string& path, Renderer* renderer)
{
	// 애니메이터 컴포넌트에 연결된 컨트롤러를 가져오자.
	// \\을 /로 바꿔서 로드한다.
	std::string pathStr = path;
	std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

	if (_controllers.find(pathStr) != _controllers.end())
	{
		return &_controllers.at(pathStr);
	}

	// 없으면 로드하자.
	std::filesystem::path controllerPath = pathStr;

	if (!std::filesystem::exists(controllerPath))
	{
		return nullptr;
	}

	if (controllerPath.extension() != ".controller")
	{
		return nullptr;
	}

	AnimatorController controller = AnimatorController::Load(controllerPath.string());

	// 여기서 모션을 로드하자.
	for (auto& [stateName, state] : controller.stateMap)
	{
		if (state.motionName.empty())
			continue;

		if (renderer == nullptr)
		{
			continue;
		}
		state.motion = renderer->GetAnimationClip(state.motionName);
	}

	_controllers.emplace(pathStr, std::move(controller));

	return &_controllers.at(pathStr);
}

core::AnimatorController* core::ControllerManager::GetController(const std::string& path)
{
	if (_controllers.find(path) != _controllers.end())
	{
		return &_controllers.at(path);
	}

	return nullptr;
}

core::AnimatorController* core::ControllerManager::GetController(const std::string& path, Renderer* renderer)
{
	if (_controllers.find(path) != _controllers.end())
	{
		return &_controllers.at(path);
	}

	return LoadController(path, renderer);
}