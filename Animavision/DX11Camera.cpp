#include "pch.h"
#include "DX11Camera.h"

DX11Camera::DX11Camera()
{
	SetLens(0.25f * DirectX::XM_PI, static_cast<float>(1920) / 1080, 0.1f, 1000.0f);
}

// WOO 나중에 만들기
DX11Camera::DX11Camera(const GlobalData& data)
{

}

DX11Camera::~DX11Camera()
{

}

void DX11Camera::Update()
{
	float dt = 0.016f;
	// camera movement
	if (GetAsyncKeyState('W') & 0x8000)
		Walk(100.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		Walk(-100.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		Strafe(-100.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		Strafe(100.0f * dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		WorldUpDown(-100.0f * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		WorldUpDown(100.0f * dt);

	// camera rotation
	if (GetAsyncKeyState(VK_UP) & 0x8000)
		Pitch(0.1f * dt);

	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		Pitch(-0.1f * dt);

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		RotateY(-0.1f * dt);

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		RotateY(0.1f * dt);

}

void DX11Camera::UpdateViewMatrix()
{
	DirectX::XMVECTOR R = DirectX::XMLoadFloat3(&m_Right);
	DirectX::XMVECTOR U = DirectX::XMLoadFloat3(&m_Up);
	DirectX::XMVECTOR L = DirectX::XMLoadFloat3(&m_Look);
	DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&m_Position);

	// Keep camera's axes orthogonal to each other and of unit length.
	L = DirectX::XMVector3Normalize(L);
	U = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(L, R));

	// U, L already ortho-normal, so no need to normalize cross product.
	R = DirectX::XMVector3Cross(U, L);

	// Fill in the view matrix entries.
	float x = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, R));
	float y = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, U));
	float z = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, L));

	DirectX::XMStoreFloat3(&m_Right, R);
	DirectX::XMStoreFloat3(&m_Up, U);
	DirectX::XMStoreFloat3(&m_Look, L);

	m_View(0, 0) = m_Right.x;
	m_View(1, 0) = m_Right.y;
	m_View(2, 0) = m_Right.z;
	m_View(3, 0) = x;

	m_View(0, 1) = m_Up.x;
	m_View(1, 1) = m_Up.y;
	m_View(2, 1) = m_Up.z;
	m_View(3, 1) = y;

	m_View(0, 2) = m_Look.x;
	m_View(1, 2) = m_Look.y;
	m_View(2, 2) = m_Look.z;
	m_View(3, 2) = z;

	m_View(0, 3) = 0.0f;
	m_View(1, 3) = 0.0f;
	m_View(2, 3) = 0.0f;
	m_View(3, 3) = 1.0f;
}

void DX11Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	m_Fov = fovY;
	m_Aspect = aspect;
	m_NearZ = zn;
	m_FarZ = zf;

	m_NearWindowHeight = 2.0f * m_NearZ * tanf(0.5f * m_Fov);
	m_FarWindowHeight = 2.0f * m_FarZ * tanf(0.5f * m_Fov);

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(m_Fov, m_Aspect, m_NearZ, m_FarZ);
	m_Projection = P;
}

void DX11Camera::Strafe(float d)
{
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR r = DirectX::XMLoadFloat3(&m_Right);
	DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&m_Position);
	m_Position = DirectX::XMVectorMultiplyAdd(s, r, p);
}

void DX11Camera::Walk(float d)
{
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR l = DirectX::XMLoadFloat3(&m_Look);
	DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&m_Position);
	m_Position = DirectX::XMVectorMultiplyAdd(s, l, p);
}

void DX11Camera::WorldUpDown(float d)
{
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR u = DirectX::XMLoadFloat3(&m_Up);
	DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&m_Position);
	m_Position = DirectX::XMVectorMultiplyAdd(s, u, p);
}

void DX11Camera::Pitch(float angle)
{
	Matrix R = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&m_Right), angle);

	DirectX::XMStoreFloat3(&m_Up, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&m_Up), R));
	DirectX::XMStoreFloat3(&m_Look, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&m_Look), R));
}

void DX11Camera::RotateY(float angle)
{
	Matrix R = DirectX::XMMatrixRotationY(angle);

	DirectX::XMStoreFloat3(&m_Right, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&m_Right), R));
	DirectX::XMStoreFloat3(&m_Up, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&m_Up), R));
	DirectX::XMStoreFloat3(&m_Look, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&m_Look), R));
}

void DX11Camera::LookAt(const Vector3& pos, const Vector3& target, const Vector3& up)
{
	DirectX::XMVECTOR L = DirectX::XMVector3Normalize(target - pos);
	DirectX::XMVECTOR R = DirectX::XMVector3Normalize(XMVector3Cross(up, L));
	DirectX::XMVECTOR U = DirectX::XMVector3Cross(L, R);

	DirectX::XMStoreFloat3(&m_Position, pos);
	DirectX::XMStoreFloat3(&m_Look, L);
	DirectX::XMStoreFloat3(&m_Right, R);
	DirectX::XMStoreFloat3(&m_Up, U);
}
