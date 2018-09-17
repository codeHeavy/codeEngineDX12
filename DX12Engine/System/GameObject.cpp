#include "GameObject.h"

GameObject::GameObject(Mesh*  _mesh)
{
	mesh = _mesh;
	XMMATRIX W = XMMatrixIdentity();
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(W));

	position = XMFLOAT3(0, 0, 0);
	rotation = XMFLOAT3(0, 0, 0);
	scale = XMFLOAT3(1, 1, 1);
}

GameObject::~GameObject() {

}

XMFLOAT4X4 GameObject::GetWorldMatrix() 
{
	return worldMatrix;
}

XMFLOAT3 GameObject::GetPosition()
{
	return position;
}

XMFLOAT3 GameObject::GetRotation()
{
	return rotation;
}

XMFLOAT3 GameObject::GetScale()
{
	return scale;
}

Mesh* GameObject::GetMesh()
{
	return mesh;
}

void GameObject::SetWorldMatrix(XMMATRIX wrldMatrix)
{
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(wrldMatrix));
}

void GameObject::SetPosition(XMFLOAT3 pos)
{
	position = pos;
}

void GameObject::SetRotation(XMFLOAT3 rot)
{
	rotation = rot;
}

void GameObject::SetScale(XMFLOAT3 _scale)
{
	scale = _scale;
}

void GameObject::UpdateWorldMatrix()
{
	XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX rotMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMMATRIX translateMatrix = XMMatrixTranslation(position.x, position.y, position.z);
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose( scaleMatrix* rotMatrix * translateMatrix));
}

void GameObject::Move(XMVECTOR velocityVector)
{
	XMVECTOR pos = XMVectorSet(position.x, position.y, position.z, 0);
	pos += velocityVector;
	XMStoreFloat3(&position, pos);
	XMMATRIX translateMatrix = XMMatrixTranslation(position.x, position.y, position.z);
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(translateMatrix));
}

void GameObject::Move(float x, float y, float z)
{
	position.x += x;
	position.y += y;
	position.z += z;
}

void GameObject::Rotate(float x, float y, float z)
{
	rotation.x += x;
	rotation.y += y;
	rotation.z += z;
}