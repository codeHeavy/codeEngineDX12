#pragma once
#include <DirectXMath.h>
using namespace DirectX;

struct Vertex {
		Vertex(float x, float y, float z, float u, float v) : position(x, y, z), uv(u, v) {}
		Vertex() {};
		XMFLOAT3 position;
		XMFLOAT2 uv;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		float padding[5];
};