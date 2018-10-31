#pragma once
#include "../stdafx.h"

using namespace DirectX;
class Camera
{
private:
	XMFLOAT4X4 projectionMatrix;
	XMFLOAT4X4 viewMatrix;

	XMFLOAT3 position;
	XMFLOAT3 direction;
	XMFLOAT4 rotation;
	XMFLOAT3 up;

	float rotationX;
	float rotationY;
	float speed;
	float camSensitivity;
public:
	Camera();
	~Camera();

	void SetProjectionMatrix( XMFLOAT4X4 projMat );
	void SetViewMatrix( XMFLOAT4X4 viewMat );
	void SetPosition(XMFLOAT3 pos);
	void SetDirection(XMFLOAT3 dir);

	XMFLOAT4X4 GetProjectionMatrix();
	XMFLOAT4X4 GetViewMatrix();

	XMFLOAT3 GetPosition();
	XMFLOAT3 GetDirection();
	XMFLOAT3 GetUp();

	void Update();
	void Update(float deltaTime);
	void UpdateProjectionMatrix(int width, int height);

	// Transformations
	void MoveRelative(float x, float y, float z);
	void MoveAbsolute(float x, float y, float z);
	void Rotate(float x, float y);
};

