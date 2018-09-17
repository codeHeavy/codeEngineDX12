#pragma once
#include "../stdafx.h"
#include "Mesh.h"
using namespace DirectX;
class GameObject
{
private:
	XMFLOAT4X4 worldMatrix;
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	XMFLOAT3 scale;
	Mesh* mesh;
public:
	GameObject(Mesh* mesh);
	~GameObject();

	XMFLOAT4X4 GetWorldMatrix();
	XMFLOAT3 GetPosition();
	XMFLOAT3 GetRotation();
	XMFLOAT3 GetScale();
	Mesh* GetMesh();

	void SetWorldMatrix(XMMATRIX wrldMatrix);
	void SetPosition(XMFLOAT3 pos);
	void SetRotation(XMFLOAT3 rot);
	void SetScale(XMFLOAT3 _scale);

	void Move(XMVECTOR velocityVector);
	void Move(float x, float y, float z);
	void Rotate(float x, float y, float z);
	void Scale(float x, float y, float z);
	void UpdateWorldMatrix();
};