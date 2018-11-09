#include "Camera.h"



Camera::Camera()
{
	position = DirectX::XMFLOAT3(0, 0, -10);
	direction = DirectX::XMFLOAT3(0, 0, 1);
	up = DirectX::XMFLOAT3(0, 1, 0);
	XMStoreFloat4(&rotation, XMQuaternionIdentity());
	rotationX = 0;
	rotationY = 0;
	speed = 25;
	camSensitivity = 0.005;

	XMStoreFloat4x4(&viewMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&projectionMatrix, XMMatrixIdentity());
}


Camera::~Camera()
{
}

void Camera::SetProjectionMatrix(XMFLOAT4X4 projMat )
{
	projectionMatrix = projMat;
}

void Camera::SetViewMatrix(XMFLOAT4X4 viewMat)
{
	viewMatrix = viewMat;
}

void Camera::SetPosition(XMFLOAT3 pos)
{
	position = pos;
}

void Camera::SetDirection(XMFLOAT3 dir)
{
	direction = dir;
}

XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	return projectionMatrix;
}

XMFLOAT4X4 Camera::GetViewMatrix()
{
	XMVECTOR dir = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), XMLoadFloat4(&rotation));

	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&position),
		dir,
		XMVectorSet(0, 1, 0, 0));

	XMStoreFloat4x4(&viewMatrix, view);
	return viewMatrix;
}

XMFLOAT3 Camera::GetPosition()
{
	return position;
}

XMFLOAT3 Camera::GetDirection()
{
	return direction;
}

XMFLOAT3 Camera::GetUp()
{
	return up;
}

void Camera::Update()
{
	XMVECTOR dir = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), XMLoadFloat4(&rotation));

	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&position),
		dir,
		XMVectorSet(0, 1, 0, 0));

	XMStoreFloat4x4(&viewMatrix, view);
}

void Camera::Update(float deltaTime)
{
	// Current speed
	float speed = deltaTime * 3;

	// Speed up or down as necessary
	if (GetAsyncKeyState(VK_SHIFT)) { speed *= 5; }
	if (GetAsyncKeyState(VK_CONTROL)) { speed *= 0.1f; }

	// Movement
	if (GetAsyncKeyState('W') & 0x8000) { MoveRelative(0, 0, speed); }
	if (GetAsyncKeyState('S') & 0x8000) { MoveRelative(0, 0, -speed); }
	if (GetAsyncKeyState('A') & 0x8000) { MoveRelative(-speed, 0, 0); }
	if (GetAsyncKeyState('D') & 0x8000) { MoveRelative(speed, 0, 0); }
	if (GetAsyncKeyState('X') & 0x8000) { MoveAbsolute(0, -speed, 0); }
	if (GetAsyncKeyState(' ') & 0x8000) { MoveAbsolute(0, speed, 0); }
	Update();
}

void Camera::UpdateProjectionMatrix(int width, int height)
{
	XMMATRIX tmpMat = XMMatrixPerspectiveFovLH(45.0f*(3.14f / 180.0f), (float)width / (float)height, 0.1f, 1000.0f);
	XMStoreFloat4x4(&projectionMatrix, tmpMat);
}

// Moves the camera relative to its orientation
void Camera::MoveRelative(float x, float y, float z)
{
	// Rotate the desired movement vector
	XMVECTOR dir = XMVector3Rotate(XMVectorSet(x, y, z, 0), XMLoadFloat4(&rotation));

	// Move in that direction
	XMStoreFloat3(&position, XMLoadFloat3(&position) + dir);
}

// Moves the camera in world space (not local space)
void Camera::MoveAbsolute(float x, float y, float z)
{
	// Simple add, no need to load/store
	position.x += x;
	position.y += y;
	position.z += z;
}

// Rotate on the X and/or Y axis
void Camera::Rotate(float x, float y)
{
	// Adjust the current rotation
	rotationX += x;
	rotationY += y;

	// Clamp the x between PI/2 and -PI/2
	rotationX = max(min(rotationX, XM_PIDIV2), -XM_PIDIV2);

	// Recreate the quaternion
	XMStoreFloat4(&rotation, XMQuaternionRotationRollPitchYaw(rotationX, rotationY, 0));
}