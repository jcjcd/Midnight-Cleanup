#pragma once
#include "SystemInterface.h"
#include "SystemTraits.h"
#include "NavConfigs.h"

class dtCrowd;

namespace core
{
	struct OnFinishSystem;
	struct OnStartSystem;
	struct NavMeshData;
	struct Agent;

	class PathFindingSystem : public ISystem, public IUpdateSystem
	{
		struct AgentData
		{
			int index = -1;			// crowd 인덱스
			Vector3 destination;	// 목적지
			bool isStopped;			// 정지 여부
		};

	public:
		PathFindingSystem(Scene& scene);
		~PathFindingSystem() override;

		void operator()(Scene& scene, float tick) override;

	private:
		// 초기화
		bool initCrowd(int maxAgents, float maxAgentRadius);

		// 에이전트 생성
		int placeAgentOnMesh(Agent& agent, const Vector3& startPos, entt::entity entity);
		// 에이전트 업데이트
		void updateAgentParam(entt::registry& registry, entt::entity entity);
		// 목적지 업데이트
		void updateDestination(core::Agent& agent, entt::entity entity);

		// 이벤트
		void startSystem(const OnStartSystem& event);
		void finishSystem(const OnFinishSystem& event);

		entt::entity getEntityFromUserData(void* userData);

		entt::dispatcher* _dispatcher = nullptr;

		NavMeshData _navMeshData;
		dtCrowd* _crowd = nullptr;

		std::map<entt::entity, AgentData> _agentData;
	};
}
DEFINE_SYSTEM_TRAITS(core::PathFindingSystem)
