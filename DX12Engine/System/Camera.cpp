#include "Camera.h"



Camera::Camera()
{
	position = DirectX::XMFLOAT3(0, 0, -10);
	direction = DirectX::XMFLOAT3(0, 0, 1);
	up = DirectX::XMFLOAT3(0, 1, 0);
	rotationX = 0;
	rotationY = 0;
	speed = 25;
	camSensitivity = 0.005;
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
	XMVECTOR cPos = XMLoadFloat3(&position);
	XMVECTOR cTarg = XMLoadFloat3(&direction);
	XMVECTOR cUp = XMLoadFloat3(&up);
	XMVECTOR rotVector = DirectX::XMQuaternionRotationRollPitchYaw(rotationY, rotationX, 0);		// rotX is rotation along Y-axis
	XMVECTOR r = DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&direction), rotVector);		// May cause error - may need pointer
	XMMATRIX tmpMat = XMMatrixLookAtLH(cPos, r, cUp);
	XMStoreFloat4x4(&viewMatrix, tmpMat);
}

void Camera::UpdateProjectionMatrix(int width, int height)
{
	XMMATRIX tmpMat = XMMatrixPerspectiveFovLH(45.0f*(3.14f / 180.0f), (float)width / (float)height, 0.1f, 1000.0f);
	XMStoreFloat4x4(&projectionMatrix, tmpMat);
}
