#pragma once
#include "../stdafx.h"


using namespace DirectX;

// Constant buffers muct be 256 byte aligned
struct ConstantBuffer
{
	XMFLOAT4X4 worldViewProjectionMatrix;
};
