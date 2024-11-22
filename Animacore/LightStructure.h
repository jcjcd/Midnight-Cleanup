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
		// ī�޶��� �����
		Matrix viewInverse = view.Invert();

		// �þ߰��� �̿��ؼ� ���� �þ߰��� ����
		//float tanHalfFov = std::tanf(DirectX::XMConvertToRadians(fov / 2.0f));	// �̹� radian�� �� ���Ƽ� �̰� �ϴ� ����
		float tanHalfVerticalFov = std::tanf(fov / 2.0f);
		// ���� �þ߰��� �̿��Ͽ� ���� �þ߰��� ����
		float tanHalfHorizontalFov = tanHalfVerticalFov * aspectValue;

		// ����ü�� ������ ���� �� �κ� ����ü�� �� ���� ����
		std::vector<float> cascadeEnds;
		cascadeEnds.reserve(numCascades + 1);
		cascadeEnds.push_back(nearPlane);
		cascadeEnds.push_back((farPlane - nearPlane) * cascadeEndValues.x);
		cascadeEnds.push_back((farPlane - nearPlane) * cascadeEndValues.y);
		cascadeEnds.push_back((farPlane - nearPlane) * cascadeEndValues.z);
		cascadeEnds.push_back(farPlane);

		// 4���� ����ü�� ������ ���� 4�� �ݺ���
		for (int i = 0; i < 4; i++)
		{
			//  +X, +Y ��ǥ�� ����, ���� �þ߰��� �̿��Ͽ� ����. �� �κ� ����ü�� �����, �� ����� ���� ���Ͽ� 4���� ���� ����
			float xn = cascadeEnds[i] * tanHalfHorizontalFov;
			float xf = cascadeEnds[i + 1] * tanHalfHorizontalFov;
			float yn = cascadeEnds[i] * tanHalfVerticalFov;
			float yf = cascadeEnds[i + 1] * tanHalfVerticalFov;

			// +��ǥ���� ���ϸ� -��ǥ���� ���Ͽ� ������ ����ü ����� ���� �� ����. �� ����ü�� Z���� �����Ͽ� i�� ���� ������ ����� ���, �� ����� ������
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

			// frustumCorners�� ī�޶� �� ������� ����Ͽ� ���� ��ǥ�� ��ȯ
			Vector4 centerPos = Vector4::Zero;
			for (int i = 0; i < 8; i++)
			{
				frustumCorners[i] = Vector4::Transform(frustumCorners[i], viewInverse);
				centerPos += frustumCorners[i];
			}
			centerPos /= 8.0f;

			// �� ����ü �ڳʿ� �߽� ���� �Ÿ��� �ִ밪�� ���Ͽ� ���� �������� ����
			float radius = 0.0f;
			for (auto frustumCorner : frustumCorners)
			{
				float distance = (centerPos - frustumCorner).Length();
				radius = std::max<float>(radius, distance);
			}

			radius = std::ceil(radius * 16.0f) / 16.0f;

			// ���� �������� �̿��Ͽ� AABB�� ������
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
		// �� ���������� �� ��ȯ�� ����
		Vector3 targets[6] =
		{
			{ 1.0f, 0.0f, 0.0f },	// +X
			{ -1.0f, 0.0f, 0.0f },	// -X
			{ 0.0f, 1.0f, 0.0f },	// +Y
			{ 0.0f, -1.0f, 0.0f },	// -Y
			{ 0.0f, 0.0f, 1.0f },	// +Z
			{ 0.0f, 0.0f, -1.0f }	// -Z			
		};

		// �� ���⿡ ���� �� ���͸� ����
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