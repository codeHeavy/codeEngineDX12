#pragma once
#include "../stdafx.h"
#include "Vertex.h"

class Mesh
{
private:
	ID3D12Resource* vertexBuffer;
	ID3D12Resource* indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	int numVerts;
	int numIndices;
	int sizeVertBuffer;
	int sizeIndexBuffer;

	void CreateVertexBuffer(Vertex* vertexList, ID3D12Device1* device, ID3D12GraphicsCommandList* commandList);
	void CreateIndexBuffer(unsigned int* indexList, ID3D12Device1* device, ID3D12GraphicsCommandList* commandList);
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);
public:
	Mesh(Vertex* vertexList, int numverts, unsigned int* indexList,   int numindices, ID3D12Device1* device, ID3D12GraphicsCommandList* commandList);
	Mesh(const char* objFile, ID3D12Device1* device, ID3D12GraphicsCommandList* commandList);
	~Mesh();
	ID3D12Resource* GetVertexBuffer();
	ID3D12Resource* GetIndexBuffer();
	int GetNumVerts();
	int GetNumIndices();
	int GetVertSize();
	int GetIndexSize();
	D3D12_VERTEX_BUFFER_VIEW GetvBufferView();
	D3D12_INDEX_BUFFER_VIEW GetiBufferView();
};