#pragma once
#include "../stdafx.h"
#include "Lights.h"

using namespace DirectX;

// Constant buffers muct be 256 byte aligned
struct ConstantBuffer
{
	XMFLOAT4X4 worldViewProjectionMatrix;
	XMFLOAT4X4 worldMatrix;
};

struct PSConstantBuffer
{
	DirectionalLight light;
	PointLight pLight;
	XMFLOAT3 CamPos;
};

struct MotionBlurConst
{
	XMFLOAT4X4 preViewProjection;
};