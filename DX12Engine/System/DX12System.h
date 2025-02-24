#pragma once
#include "../stdafx.h"
#include "Vertex.h"
#include "ConstantBuffer.h"
#include "Mesh.h"
#include "GameObject.h"
#include "Camera.h"
#include "Lights.h"
#include "DefferedRenderer.h"
#include "../Texture.h"
#include "WICTextureLoader.h"
#include "ResourceUploadBatch.h"
#include "DirectXHelpers.h"
#include "Windowsx.h"
#include "PostProcessFilter.h"
#include <vector>
#include "MotionBlur.h"

#ifndef BUFFERCOUNT
#define BUFFERCOUNT 3
#endif // frame buffer count


const int frameBufferCount = BUFFERCOUNT;
class DX12System
{
protected:
	DX12System();
	~DX12System();
public:
	// Static instance
	static DX12System* dxInstance;

	// Create static instance
	static DX12System* GetInstance();
	static void ResetInstance()
	{
		delete dxInstance; // REM : it works even if the pointer is NULL (does nothing then)
		dxInstance = nullptr; // so GetInstance will still work.
	}
	// Window handles
	HWND hwnd = NULL;						// Windows handle
	LPCWSTR windowName = L"codeEngineDX12";		// Window name
	LPCWSTR windowTitle = L"codeEngineDX12";	// Window title

	// Size of the window
	int height = 1024;
	int width = 720;

	// Window properties
	bool fullscreen = false;

	bool InitWindow(HINSTANCE hInstance, int showWnd, int _width, int _height, bool fullscreen);

	void UpdateLoop();

	static LRESULT CALLBACK WindowProc(
		HWND hWnd,		// Window handle
		UINT uMsg,		// Message
		WPARAM wParam,	// Message's first parameter
		LPARAM lParam	// Message's second parameter
	);

	// Callback to handle windows messges
	LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Direct X 12 Setup
	ID3D12Device1* device;
	IDXGIAdapter1* adapter;
	IDXGIFactory4* dxgiFactory;
	IDXGISwapChain3* swapChain;
	ID3D12CommandQueue* commandQueue;							// Command list container
	ID3D12DescriptorHeap* rtvDescrioptorHeap;					// holds resources like render target
	ID3D12Resource* renderTargets[BUFFERCOUNT];					// Number of render targets = buffer count
	ID3D12CommandAllocator* commandAllocator[BUFFERCOUNT];		// Allocators = buffer * number of threads ( 1 thread used )
	ID3D12GraphicsCommandList* commandList;						// Store commands
	ID3D12Fence* fence[BUFFERCOUNT];							// used for CPU-GPU sync. number of fences = number of allocators
	HANDLE fenceEvent;											// Handle to when fence is unlocked by GPU
	UINT64 fenceValue[BUFFERCOUNT];								// incremented each frame, each fence will have its own value
	int frameIndex;												// current RTV
	int rtvDescriptorSize;										// size of RTV descriptor (front and back buffers will be of the same size)
	
	bool InitD3D();												// Initialize direct3D 12
	void Update();												// Update game logic
	void UpdatePipeline();										// Update direct3D pipeline
	void Render();												// render the screen
	void Cleanup();												// release and cleanup
	void WaitForPreviousFrame();								// wait for GPU to sync wityh CPU
	bool SetupResources();										// Setup resources to draw
	void Draw();												// Set command lists to draw

	int adapterIndex = 0;
	bool adapterFound = false;
	bool running = true;

	// To draw objects
	ID3D12PipelineState* pipelineStateObject;					// pso containing a pipeline state
	ID3D12RootSignature* rootSignature;
	D3D12_VIEWPORT viewport;
	D3D12_RECT	scissorRect;									// cuttoff rectangle
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;					// struct of vertex data in GPU mem
	DXGI_SAMPLE_DESC sampleDesc;

	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	// Depth stencil
	ID3D12Resource* depthStencilBuffer;
	ID3D12DescriptorHeap* dsDescriptorHeap;

	ID3D12Resource* constantBufferUploadHeap[BUFFERCOUNT];		// mem on GPU for constant buffer
	ConstantBuffer constantBufferPerObject;
	UINT8* constantBufferGPUAddress[BUFFERCOUNT];				// pointer to memory location

	int numCubeIndices;

	void BuildViewProjMatrix();

	ID3D12Resource* skyTextureBuffer;
	ID3D12Resource* skyIRTextureBuffer;
	ID3D12Resource* skyPreFilterTextureBuffer;
	ID3D12Resource* skyBrdfTextureBuffer;
	DefferedRenderer* deferredRenderer;

	double perfCounterSeconds;
	float totalTime;
	float deltaTime;
	__int64 startTime;
	__int64 currentTime;
	__int64 previousTime;

	// FPS calculation
	int fpsFrameCount;
	float fpsTimeElapsed;
	void UpdateTimer();

	Camera* camera;
	Mesh* mesh;
	GameObject* cube1;
	PSConstantBuffer PSCBuffer;

	// Convenience methods for handling mouse input, since we
	// can easily grab mouse input from OS-level messages
	void OnMouseDown	(WPARAM buttonState, int x, int y);
	void OnMouseUp	(WPARAM buttonState, int x, int y);
	void OnMouseMove	(WPARAM buttonState, int x, int y);
	void OnMouseWheel(float wheelDelta, int x, int y);
	POINT prevMousePos;

	// Input and mesh swapping
	byte keys[256];
	byte prevKeys[256];

	void Loadtextures();
	std::vector<Texture*> albedoList;
	std::vector<Texture*> normalList;
	std::vector<Texture*> roughnessList;
	std::vector<Texture*> metalList;

	bool KeyDown(int key);
	bool KeyPressed(int key);
	bool KeyReleased(int key);
	int textureIndex = 0;

	std::vector<PostProcessFilter*> postProcessFilters;
	DirectX::XMFLOAT4X4 prevViewProj;
	ID3D12Resource* prevWorldPos;
	ID3D12Resource* currentWorldPos;

	MotionBlur* motionBlur;
};