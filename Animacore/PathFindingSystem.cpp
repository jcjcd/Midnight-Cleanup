#include "pch.h"
#include "PathFindingSystem.h"

#include "Scene.h"
#include "CoreComponents.h"

#include <recastnavigation/DetourCrowd.h>

#include "NavConfigs.h"
#include "NavMeshBuilder.h"
#include "PhysicsScene.h"

constexpr int MAX_AGENTS = 64;
constexpr float MAX_AGENT_RADIUS = 5.0f;

core::PathFindingSystem::PathFindingSystem(Scene& scene)
	: ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<OnStartSystem>().connect<&PathFindingSystem::startSystem>(this);
	_dispatcher->sink<OnFinishSystem>().connect<&PathFindingSystem::finishSystem>(this);
}

core::PathFindingSystem::~PathFindingSystem()
{
	_dispatcher->disconnect(this);
}

void core::PathFindingSystem::operator()(Scene& scene, float tick)
{
	if (!scene.IsPlaying())
		return;

	if (_crowd)
		_crowd->update(tick, nullptr);

	auto& registry = *scene.GetRegistry();
	auto view = registry.view<Agent, WorldTransform>();
	dtCrowdAgent* agents[MAX_AGENTS];
	auto cnt = _crowd->getActiveAgents(agents, MAX_AGENTS);

	for (int i = 0; i < cnt; ++i)
	{
		auto entity = getEntityFromUserData(agents[i]->params.userData);
		auto& world = view.get<WorldTransform>(entity);
		world.position.x = agents[i]->npos[0];
		world.position.y = agents[i]->npos[1];
		world.position.z = agents[i]->npos[2];
		registry.patch<WorldTransform>(entity);
	}
}

int core::PathFindingSystem::placeAgentOnMesh(Agent& agent, const Vector3& startPos, entt::entity entity)
{
	// startPos에 해당하는 폴리곤을 탐색
	Vector3 closestPos;
	Vector3 extents = { 2.f, 4.f, 2.f };
	auto status = _navMeshData.navMeshQuery->findNearestPoly(&startPos.x, &extents.x, _crowd->getFilter(0), &agent._polyRef, &closestPos.x);

	if (dtStatusFailed(status))
		return -1;

	// 에이전트 추가
	dtCrowdAgentParams params = {};
	params.radius = agent.radius;
	params.height = agent.height;
	params.maxSpeed = agent.speed;
	params.maxAcceleration = agent.acceleration;
	params.collisionQueryRange = agent.collisionQueryRange;
	params.pathOptimizationRange = params.radius * 30.0f;
	params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OBSTACLE_AVOIDANCE;
	params.userData = reinterpret_cast<void*>(entity);

	return _crowd->addAgent(&closestPos.x, &params);
}

bool core::PathFindingSystem::initCrowd(int maxAgents, float maxAgentRadius)
{
	// Crowd 객체 생성 및 초기화
	_crowd = dtAllocCrowd();  // dtCrowd 할당

	if (!_crowd)
		return false;

	// Crowd 초기화: 최대 에이전트 수와 각 에이전트의 최대 반지름 설정
	if (!_crowd->init(maxAgents, maxAgentRadius, _navMeshData.navMesh))
		return false;

	// 필터 설정 (필요 시 사용자 정의 필터 사용)
	dtQueryFilter filter;
	_crowd->getEditableFilter(0)->setIncludeFlags(0xffff);  // 모든 폴리곤 포함
	_crowd->getEditableFilter(0)->setExcludeFlags(0);       // 제외할 폴리곤 없음

	return true;
}

void core::PathFindingSystem::updateAgentParam(entt::registry& registry, entt::entity entity)
{
	if (!_crowd || !_agentData.contains(entity))
		return;

	if (auto agent = registry.try_get<Agent>(entity))
	{
		if (_agentData[entity].index < 0)
			return;

		// 에이전트의 현재 상태를 _agentData에서 가져옴
		auto& agentData = _agentData[entity];

		// 정지 상태가 변경된 경우에만 처리
		if (agent->isStopped != agentData.isStopped)
		{
			agentData.isStopped = agent->isStopped;
			auto* crowdAgent = _crowd->getEditableAgent(_agentData[entity].index);

			if (agent->isStopped)
				crowdAgent->active = false;
			else
				crowdAgent->active = true;

			return;
		}

		// 목적지 변경 확인 및 업데이트
		if (agent->destination != agentData.destination)
		{
			updateDestination(*agent, entity);
			agentData.destination = agent->destination;
			return;
		}

		// 에이전트의 파라미터 업데이트
		dtCrowdAgentParams params = {};
		params.radius = agent->radius;
		params.height = agent->height;
		params.maxSpeed = agent->speed;
		params.maxAcceleration = agent->acceleration;
		params.collisionQueryRange = agent->collisionQueryRange;
		params.pathOptimizationRange = params.radius * 30.0f;
		params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OBSTACLE_AVOIDANCE;
		params.userData = reinterpret_cast<void*>(entity);

		_crowd->updateAgentParameters(_agentData[entity].index, &params);
	}
}


void core::PathFindingSystem::updateDestination(core::Agent& agent, entt::entity entity)
{
	Vector3 nearestPt;

	// 가장 가까운 폴리곤을 찾음
	dtStatus status = _navMeshData.navMeshQuery->findNearestPoly(&agent.destination.x, _crowd->getQueryExtents(), _crowd->getFilter(0), &agent._polyRef, &nearestPt.x);

	if (dtStatusFailed(status))
		return;

	// 에이전트가 유효한 경우 목표로 이동하도록 요청
	if (auto* crowdAgent = _crowd->getAgent(_agentData[entity].index))
		_crowd->requestMoveTarget(_agentData[entity].index, agent._polyRef, &nearestPt.x);
}


void core::PathFindingSystem::startSystem(const OnStartSystem& event)
{
	event.scene->GetRegistry()->on_update<Agent>().connect<&PathFindingSystem::updateAgentParam>(this);
	auto settingView = event.scene->GetRegistry()->view<NavMeshSettings>();

	NavMeshBuilder builder(event.scene);

	for (auto&& [entity, settings] : settingView.each())
	{
		auto psScene = event.scene->GetPhysicsScene();
		std::vector<float> vertices;
		std::vector<int> indices;
		psScene->GetStaticPoly(vertices, indices);
		builder.InputWorld(vertices, indices);
		builder.BuildNavMesh(settings, _navMeshData);
		break;
	}

	if (!initCrowd(MAX_AGENTS, MAX_AGENT_RADIUS))
	{
		LOG_ERROR(*event.scene, "PathFindingSystem\nFailed to Initialize Crowd");
		return;
	}

	auto registry = event.scene->GetRegistry();
	auto view = registry->view<Agent, WorldTransform>();

	for (auto&& [entity, agent, world] : view.each())
	{
		auto index = placeAgentOnMesh(agent, world.position, entity);

		if (index >= 0)
			_agentData[entity] = { index, agent.destination, agent.isStopped };
		else
			LOG_ERROR(*event.scene, "{} : Failed to add agent to the crowd.", entity);
	}
}

void core::PathFindingSystem::finishSystem(const OnFinishSystem& event)
{
	event.scene->GetRegistry()->on_update<Agent>().disconnect(this);
	_agentData.clear();
	dtFreeCrowd(_crowd);
	_navMeshData = NavMeshData{};
}

entt::entity core::PathFindingSystem::getEntityFromUserData(void* userData)
{
	return static_cast<entt::entity>(reinterpret_cast<uintptr_t>(userData));
}