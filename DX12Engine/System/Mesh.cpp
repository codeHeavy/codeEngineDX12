#include "Mesh.h"

Mesh::Mesh(Vertex* vertexList,int numverts, int* indexList, int numindices, ID3D12Device1* device, ID3D12GraphicsCommandList* commandList) {
	// numverts required
	numVerts = numverts;
	sizeVertBuffer = sizeof(Vertex) * numverts;
	// num indicies
	numIndices = numindices;
	sizeIndexBuffer = sizeof(indexList) * numindices;
	// Create Vertex Buffer
	CreateVertexBuffer(vertexList, device, commandList);
	// Create Index Buffer
	CreateIndexBuffer(indexList, device, commandList);
}

Mesh::~Mesh() {
	SAFE_RELEASE(vertexBuffer);
	SAFE_RELEASE(indexBuffer);
}

int Mesh::GetNumVerts()
{
	return numVerts;
}

int Mesh::GetNumIndices()
{
	return numIndices;
}

int Mesh::GetVertSize()
{
	return sizeVertBuffer;
}

int Mesh::GetIndexSize()
{
	return sizeIndexBuffer;
}

ID3D12Resource* Mesh::GetVertexBuffer()
{
	return vertexBuffer;
}

ID3D12Resource* Mesh::GetIndexBuffer()
{
	return indexBuffer;
}

void Mesh::CreateVertexBuffer(Vertex* vertexList, ID3D12Device1* device, ID3D12GraphicsCommandList* commandList)
{
	// Create default heap
	// Default heap = memory in GPU
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),	// default heap
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeVertBuffer),
		D3D12_RESOURCE_STATE_COPY_DEST,	// start heap in copy location
		nullptr,						// used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&vertexBuffer)
	);

	// Name for resource heaps
	vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

	// Create upload heap
	// Uplaod heap = upload data to the GPU

	// Upload vertex buffer
	ID3D12Resource* vertexBufferUploadHeap;
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeVertBuffer),
		D3D12_RESOURCE_STATE_GENERIC_READ,					// GPU will read from this and copy to default heap
		nullptr,
		IID_PPV_ARGS(&vertexBufferUploadHeap)
	);
	vertexBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	// Store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast<BYTE*>(vertexList);	// pointer to vertex array
	vertexData.RowPitch = sizeVertBuffer;					// size of all triangle data
	vertexData.SlicePitch = sizeVertBuffer;				// size of triangle data

															// Command from command list to copy data from upload heap to default heap
	UpdateSubresources(commandList, vertexBuffer, vertexBufferUploadHeap, 0, 0, 1, &vertexData);

	// transition vertex buffer data from copy destination satte to vertex buffer state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
}

void Mesh::CreateIndexBuffer(int* indexList, ID3D12Device1* device, ID3D12GraphicsCommandList* commandList)
{
	// Create default heap for index buffer
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeIndexBuffer),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&indexBuffer));

	indexBuffer->SetName(L"Index buffer resource heap");

	// create upload heap of index buffer
	ID3D12Resource* indexBufferUploadHeap;
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeIndexBuffer),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBufferUploadHeap)
	);
	indexBufferUploadHeap->SetName(L"Index Buffer Upload Rourse Heap");

	// store data in upload heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast<BYTE*>(indexList);
	indexData.RowPitch = sizeIndexBuffer;
	indexData.SlicePitch = sizeIndexBuffer;

	//command to copy the data to upload heap
	UpdateSubresources(commandList, indexBuffer, indexBufferUploadHeap, 0, 0, 1, &indexData);

	// tranmsition from copy buffer to vertex buffer
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
}