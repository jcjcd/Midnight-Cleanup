#pragma once

namespace core
{
	struct NavMeshConfig
	{
		float cellSize = 0.3f;
		float cellHeight = 0.2f;
		float walkableSlopeAngle = 30.0f;
		float walkableHeight = 0.f;

	};
}
