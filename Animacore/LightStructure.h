#pragma once

namespace core
{
	struct DirectionalLightStructure
	{
		Vector3 color = { 1.0f, 1.0f, 1.0f };
		float intensity = 1.0f;
		Vector3 direction{};
		int isOn = false;
		Vector4 cascadedEndClip;
		Matrix lightViewProjection[4]{};
		int useShadow = false;
		int lightMode = 0;
		float pads[2]{};
	};

	struct PointLightStructure
	{
		Vector3 color = { 1.0f, 1.0f, 1.0f };
		float intensity = 1.0f;
		Vector3 position{};
		float range = 100.0f;
		Vector3 attenuation = { 1.0f, 0.0f, 0.0f };
		int isOn = false;
		Matrix lightViewProjection[6]{};
		int useShadow = false;
		float nearZ = 0.5f;
		int lightMode = 0;
		float pad;
	};

	struct SpotLightStructure
	{
		Vector3 color = { 1.0f, 1.0f, 1.0f };
		float intensity = 1.0f;
		Vector3 position{};
		float range = 50.0f;
		Vector3 direction{};
		float spot = 1.0f;
		Vector3 attenuation = { 1.0f, 0.0f, 0.0f };
		int isOn = false;
		Matrix lightViewProjection{};
		float spotAngle = 60.0f;
		int useShadow = false;
		int lightMode = 0;
		float innerAngle = 30.0f;
	};

	struct LightFrustums
	{
		std::vector<DirectX::BoundingFrustum> frustums;
		entt::entity entity;
	};

	constexpr float orthoSize = 50.0f;

	static void CalculateCascadeMatrices(const Matrix& view, const Matrix& proj, const float& fov, const float& aspectValue, const Vector3& lightDir, float nearPlane, float farPlane, int numCascades, Matrix* outLightViewProjections, float* cascadedEndClip, Vector3 cascadeEndValues)
	{
		// 카메라의 역행렬
		Matrix viewInverse = view.Invert();

		// 시야각을 이용해서 수직 시야각을 구함
		//float tanHalfFov = std::tanf(DirectX::XMConvertToRadians(fov / 2.0f));	// 이미 radian인 거 같아서 이건 일단 보류
		float tanHalfVerticalFov = std::tanf(fov / 2.0f);
		// 수직 시야각을 이용하여 수평 시야각을 구함
		float tanHalfHorizontalFov = tanHalfVerticalFov * aspectValue;

		// 절두체를 나누기 위한 각 부분 절두체의 끝 지점 선언
		std::vector<float> cascadeEnds;
		cascadeEnds.reserve(numCascades + 1);
		cascadeEnds.push_back(nearPlane);
		cascadeEnds.push_back((farPlane - nearPlane) * cascadeEndValues.x);
		cascadeEnds.push_back((farPlane - nearPlane) * cascadeEndValues.y);
		cascadeEnds.push_back((farPlane - nearPlane) * cascadeEndValues.z);
		cascadeEnds.push_back(farPlane);

		// 4개의 절두체로 나누기 위해 4번 반복함
		for (int i = 0; i < 4; i++)
		{
			//  +X, +Y 좌표에 수평, 수직 시야각을 이용하여 구함. 각 부분 절두체의 가까운, 먼 평면의 값을 곱하여 4개의 점을 구함
			float xn = cascadeEnds[i] * tanHalfHorizontalFov;
			float xf = cascadeEnds[i + 1] * tanHalfHorizontalFov;
			float yn = cascadeEnds[i] * tanHalfVerticalFov;
			float yf = cascadeEnds[i + 1] * tanHalfVerticalFov;

			// +좌표값을 구하면 -좌표값을 구하여 각각의 절두체 평면을 구할 수 있음. 각 절두체의 Z값을 저장하여 i가 낮은 순서로 가까운 평면, 먼 평면을 구성함
			Vector4 frustumCorners[8] =
			{
				// near Face
				{  xn,  yn, cascadeEnds[i], 1.0f },
				{ -xn,  yn, cascadeEnds[i], 1.0f },
				{  xn, -yn, cascadeEnds[i], 1.0f },
				{ -xn, -yn, cascadeEnds[i], 1.0f },
				// far Face
				{  xf,  yf, cascadeEnds[i + 1], 1.0f },
				{ -xf,  yf, cascadeEnds[i + 1], 1.0f },
				{  xf, -yf, cascadeEnds[i + 1], 1.0f },
				{ -xf, -yf, cascadeEnds[i + 1], 1.0f }
			};

			// frustumCorners를 카메라 뷰 역행렬을 사용하여 월드 좌표로 변환
			Vector4 centerPos = Vector4::Zero;
			for (int i = 0; i < 8; i++)
			{
				frustumCorners[i] = Vector4::Transform(frustumCorners[i], viewInverse);
				centerPos += frustumCorners[i];
			}
			centerPos /= 8.0f;

			// 각 절두체 코너와 중심 간의 거리의 최대값을 구하여 구의 반지름을 구함
			float radius = 0.0f;
			for (auto frustumCorner : frustumCorners)
			{
				float distance = (centerPos - frustumCorner).Length();
				radius = std::max<float>(radius, distance);
			}

			radius = std::ceil(radius * 16.0f) / 16.0f;

			// 구의 반지름을 이용하여 AABB를 생성함
			Vector3 maxExtents = { radius, radius, radius };
			Vector3 minExtents = -maxExtents;

			float shadowMapSize = 2048.0f;
			float texelSize = 2.0f * radius / shadowMapSize;

			Vector3 shadowCamPos = Vector3(centerPos) + lightDir * minExtents.z;
			shadowCamPos.x = std::floor(shadowCamPos.x / texelSize) * texelSize;
			shadowCamPos.y = std::floor(shadowCamPos.y / texelSize) * texelSize;
			Matrix lightMatrix = DirectX::XMMatrixLookAtLH(shadowCamPos, Vector3(centerPos), Vector3::Up);
			Matrix lightProj = DirectX::XMMatrixOrthographicOffCenterLH(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);
			outLightViewProjections[i] = lightMatrix * lightProj;

			Vector4 vClip = Vector4::Transform(Vector4(0.0f, 0.0f, cascadeEnds[i + 1], 1.0f), proj);
			cascadedEndClip[i] = vClip.z;
		}
	}

	static void CreatePointLightViewProjMatrices(Vector3 lightPos, Matrix* viewProj, float nearZ, float farZ)
	{
		// 각 방향으로의 뷰 변환을 설정
		Vector3 targets[6] =
		{
			{ 1.0f, 0.0f, 0.0f },	// +X
			{ -1.0f, 0.0f, 0.0f },	// -X
			{ 0.0f, 1.0f, 0.0f },	// +Y
			{ 0.0f, -1.0f, 0.0f },	// -Y
			{ 0.0f, 0.0f, 1.0f },	// +Z
			{ 0.0f, 0.0f, -1.0f }	// -Z			
		};

		// 각 방향에 대한 업 벡터를 설정
		Vector3 ups[6] =
		{
			{ 0.0f, 1.0f, 0.0f },	// +X
			{ 0.0f, 1.0f, 0.0f },	// -X
			{ 0.0f, 0.0f, -1.0f },	// +Y
			{ 0.0f, 0.0f, 1.0f },	// -Y
			{ 0.0f, 1.0f, 0.0f },	// +Z
			{ 0.0f, 1.0f, 0.0f }	// -Z
		};

		Matrix view;
		Matrix proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 1.0f, nearZ, farZ);

		for (int i = 0; i < 6; i++)
		{
			//Vector3 targetPos = lightPos + targets[i];
			view = DirectX::XMMatrixLookToLH(lightPos, targets[i], ups[i]);
			viewProj[i] = view * proj;
		}
	}
};