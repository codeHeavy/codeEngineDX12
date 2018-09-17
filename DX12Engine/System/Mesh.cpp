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

Mesh::Mesh(const char* objFile, ID3D12Device1* device, ID3D12GraphicsCommandList* commandList)
{
	// File input object
	std::ifstream obj(objFile);

	// Check for successful open
	if (!obj.is_open())
		return;

	// Variables used while reading the file
	std::vector<XMFLOAT3> positions;     // Positions from the file
	std::vector<XMFLOAT3> normals;       // Normals from the file
	std::vector<XMFLOAT2> uvs;           // UVs from the file
	std::vector<Vertex> verts;           // Verts we're assembling
	std::vector<int> indices;           // Indices of these verts
	unsigned int vertCounter = 0;        // Count of vertices/indices
	char chars[100];                     // String for line reading

										 // Still have data left?
	while (obj.good())
	{
		// Get the line (100 characters should be more than enough)
		obj.getline(chars, 100);

		// Check the type of line
		if (chars[0] == 'v' && chars[1] == 'n')
		{
			// Read the 3 numbers directly into an XMFLOAT3
			XMFLOAT3 norm;
			sscanf_s(
				chars,
				"vn %f %f %f",
				&norm.x, &norm.y, &norm.z);

			// Add to the list of normals
			normals.push_back(norm);
		}
		else if (chars[0] == 'v' && chars[1] == 't')
		{
			// Read the 2 numbers directly into an XMFLOAT2
			XMFLOAT2 uv;
			sscanf_s(
				chars,
				"vt %f %f",
				&uv.x, &uv.y);

			// Add to the list of uv's
			uvs.push_back(uv);
		}
		else if (chars[0] == 'v')
		{
			// Read the 3 numbers directly into an XMFLOAT3
			XMFLOAT3 pos;
			sscanf_s(
				chars,
				"v %f %f %f",
				&pos.x, &pos.y, &pos.z);

			// Add to the positions
			positions.push_back(pos);
		}
		else if (chars[0] == 'f')
		{
			// Read the face indices into an array
			unsigned int i[12];
			int facesRead = sscanf_s(
				chars,
				"f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
				&i[0], &i[1], &i[2],
				&i[3], &i[4], &i[5],
				&i[6], &i[7], &i[8],
				&i[9], &i[10], &i[11]);

			// - Create the verts by looking up
			//    corresponding data from vectors
			// - OBJ File indices are 1-based, so
			//    they need to be adusted
			Vertex v1;
			v1.position = positions[i[0] - 1];
			v1.uv = uvs[i[1] - 1];
			v1.normal = normals[i[2] - 1];

			Vertex v2;
			v2.position = positions[i[3] - 1];
			v2.uv = uvs[i[4] - 1];
			v2.normal = normals[i[5] - 1];

			Vertex v3;
			v3.position = positions[i[6] - 1];
			v3.uv = uvs[i[7] - 1];
			v3.normal = normals[i[8] - 1];

			// The model is most likely in a right-handed space,
			// especially if it came from Maya.  We want to convert
			// to a left-handed space for DirectX.  This means we 
			// need to:
			//  - Invert the Z position
			//  - Invert the normal's Z
			//  - Flip the winding order
			// We also need to flip the UV coordinate since DirectX
			// defines (0,0) as the top left of the texture, and many
			// 3D modeling packages use the bottom left as (0,0)

			// Flip the UV's since they're probably "upside down"
			v1.uv.y = 1.0f - v1.uv.y;
			v2.uv.y = 1.0f - v2.uv.y;
			v3.uv.y = 1.0f - v3.uv.y;

			// Flip Z (LH vs. RH)
			v1.position.z *= -1.0f;
			v2.position.z *= -1.0f;
			v3.position.z *= -1.0f;

			// Flip normal Z
			v1.normal.z *= -1.0f;
			v2.normal.z *= -1.0f;
			v3.normal.z *= -1.0f;

			// Add the verts to the vector (flipping the winding order)
			verts.push_back(v1);
			verts.push_back(v3);
			verts.push_back(v2);

			// Add three more indices
			indices.push_back(vertCounter); vertCounter += 1;
			indices.push_back(vertCounter); vertCounter += 1;
			indices.push_back(vertCounter); vertCounter += 1;

			// Was there a 4th face?
			if (facesRead == 12)
			{
				// Make the last vertex
				Vertex v4;
				v4.position = positions[i[9] - 1];
				v4.uv= uvs[i[10] - 1];
				v4.normal = normals[i[11] - 1];

				// Flip the UV, Z pos and normal
				v4.uv.y = 1.0f - v4.uv.y;
				v4.position.z *= -1.0f;
				v4.normal.z *= -1.0f;

				// Add a whole triangle (flipping the winding order)
				verts.push_back(v1);
				verts.push_back(v4);
				verts.push_back(v3);

				// Add three more indices
				indices.push_back(vertCounter); vertCounter += 1;
				indices.push_back(vertCounter); vertCounter += 1;
				indices.push_back(vertCounter); vertCounter += 1;
			}
		}
	}

	// Close the file and create the actual buffers
	obj.close();

	//CreateBuffers(&verts[0], vertCounter, &indices[0], vertCounter, device);
	// numverts required
	numVerts = vertCounter;
	sizeVertBuffer = sizeof(Vertex) * vertCounter;
	// num indicies
	numIndices = vertCounter;
	sizeIndexBuffer = sizeof(&indices[0]) * vertCounter;
	CreateVertexBuffer(&verts[0], device, commandList);
	CreateIndexBuffer(&indices[0], device, commandList);
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

D3D12_VERTEX_BUFFER_VIEW Mesh::GetvBufferView()
{
	return vertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW Mesh::GetiBufferView()
{
	return indexBufferView;
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
	
	// create a vertex buffer view for the triangle
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = sizeVertBuffer;
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
	
	// create index buffer view
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = sizeIndexBuffer;
}