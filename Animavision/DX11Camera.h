#pragma once

#include "RendererDLL.h"
#include "DX11Relatives.h"
#include <vector>

// WOO 언젠가 엔진에서 받아올 데이터의 구조체
struct GlobalData;

/// graphics에서 계산 용으로 사용할 카메라
/// 엔진에서 사용하는 카메라 컴포넌트와 별개
/// 엔진 카메라의 데이터로 만드는게 좋을듯
/// 컬링 계산 용으로 사용해도 될듯?
class ANIMAVISION_DLL DX11Camera
{
public:
	DX11Camera();
	DX11Camera(const GlobalData& data);
	~DX11Camera();

	void Update();

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix();

	void SetLens(float fovY, float aspect, float zn, float zf);

	// Strafe/Walk the camera a distance d.
	void Strafe(float d);
	void Walk(float d);
	void WorldUpDown(float d);

	// Rotate the camera.
	void Pitch(float angle);
	void RotateY(float angle);

	// Define camera space via LookAt parameters.
	void LookAt(const Vector3& pos, const Vector3& target, const Vector3& up);

	Vector3 GetPosition() const { return m_Position; }
	void SetPosition(Vector3 val) { m_Position = val; }
	Vector3 GetRight() const { return m_Right; }
	Vector3 GetUp() const { return m_Up; }
	Vector3 GetLook() const { return m_Look; }
	float GetNear() const { return m_NearZ; }
	float GetFar() const { return m_FarZ; }
	float GetFov() const { return m_Fov; }
	float GetAspect() const { return m_Aspect; }
	float GetNearWidthHeight() const { return m_NearWindowHeight; }
	float GetFarWidthHeight() const { return m_FarWindowHeight; }
	Matrix GetView() const { return m_View; }
	Matrix GetProjection() const { return m_Projection; }
	Matrix GetViewProj() const { return m_View * m_Projection; }

private:
	Vector3 m_Position;
	Vector3 m_Right;
	Vector3 m_Up;
	Vector3 m_Look;

	float m_NearZ = 0.0f;
	float m_FarZ = 0.0f;
	float m_Fov = 0.0f;
	float m_Aspect = 0.0f;
	float m_NearWindowHeight = 0.0f;
	float m_FarWindowHeight = 0.0f;

	Matrix m_View;
	Matrix m_Projection;

	// WOO: 추후에 카메라가 여러 개 사용할 수 있도록 고치기
};

