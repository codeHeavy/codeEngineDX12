#pragma once

#include <DirectXMath.h>
using namespace DirectX;

struct Vertex {
		Vertex(float x, float y, float z, float u, float v) : position(x, y, z), texCoord(u, v) {}
		XMFLOAT3 position;
		XMFLOAT2 texCoord;
};