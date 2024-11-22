#pragma once
#include <physx/characterkinematic/PxControllerBehavior.h>


namespace mc
{
	class BehaviorCallback : public physx::PxControllerBehaviorCallback
	{
		using flags = physx::PxControllerBehaviorFlags;
		using shape = physx::PxShape;
		using actor = physx::PxActor;
		using controller = physx::PxController;
		using obstacle = physx::PxObstacle;

	public:
		flags getBehaviorFlags(const shape& shape, const actor& actor) override;
		flags getBehaviorFlags(const controller& controller) override;
		flags getBehaviorFlags(const obstacle& obstacle) override;
	};

    class HitReport : public physx::PxUserControllerHitReport
    {
		using shapeHit = physx::PxControllerShapeHit;
		using controllerHit = physx::PxControllersHit;
		using obstacleHit = physx::PxControllerObstacleHit;

    public:
	    void onShapeHit(const shapeHit& hit) override;
	    void onControllerHit(const controllerHit& hit) override;
	    void onObstacleHit(const obstacleHit& hit) override;
    };
}

