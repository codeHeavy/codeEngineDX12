#pragma once
#include "../stdafx.h"

using namespace DirectX;
struct DirectionalLight
{
	XMFLOAT4 AmbientColor;
	XMFLOAT4 DiffuseColor;
	XMFLOAT3 Direction;
	float padding;
};

struct PointLight
{
	XMFLOAT4 Color;
	XMFLOAT3 Position;
	float padding;
};