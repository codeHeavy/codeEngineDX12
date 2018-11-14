#pragma once
#include <DirectXMath.h>
using namespace DirectX;

struct Vertex {
		Vertex(XMFLOAT3 position, XMFLOAT2 uv, XMFLOAT3 normal) : position(position), uv(uv), normal(normal) {}
		Vertex(float x, float y, float z, float u, float v) : position(x, y, z), uv(u, v) {}
		Vertex() {};
		XMFLOAT3 position;
		XMFLOAT2 uv;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		float padding[5];
};