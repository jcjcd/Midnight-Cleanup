#include "pch.h"
#include "CustomCallbacks.h"

mc::BehaviorCallback::flags mc::BehaviorCallback::getBehaviorFlags(const shape& shape, const actor& actor)
{
	return flags(0);
}

mc::BehaviorCallback::flags mc::BehaviorCallback::getBehaviorFlags(const controller& controller)
{
	return flags(0);
}

mc::BehaviorCallback::flags mc::BehaviorCallback::getBehaviorFlags(const obstacle& obstacle)
{
	return flags(0);
}

void mc::HitReport::onShapeHit(const shapeHit& hit)
{
}

void mc::HitReport::onControllerHit(const controllerHit& hit)
{
}

void mc::HitReport::onObstacleHit(const obstacleHit& hit)
{
}
