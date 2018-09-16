#pragma once
#include "../stdafx.h"
#include "Vertex.h"

class Mesh
{
private:
	ID3D12Resource* vertexBuffer;
	ID3D12Resource* indexBuffer;
	int numVerts;
	int numIndices;
	int sizeVertBuffer;
	int sizeIndexBuffer;

	void CreateVertexBuffer(Vertex* vertexList, ID3D12Device1* device, ID3D12GraphicsCommandList* commandList);
	void CreateIndexBuffer(int* indexList, ID3D12Device1* device, ID3D12GraphicsCommandList* commandList);
public:
	Mesh(Vertex* vertexList, int numverts, int* indexList,  int numindices, ID3D12Device1* device, ID3D12GraphicsCommandList* commandList);
	~Mesh();
	ID3D12Resource* GetVertexBuffer();
	ID3D12Resource* GetIndexBuffer();
	int GetNumVerts();
	int GetNumIndices();
	int GetVertSize();
	int GetIndexSize();
};