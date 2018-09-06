#pragma once
#include "../stdafx.h"

#ifndef BUFFERCOUNT
#define BUFFERCOUNT 3
#endif // frame buffer count

#define SAFE_RELEASE(x) { if(x) { x->Release(); x = 0; } }
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

	// Window handles
	HWND hwnd = NULL;						// Windows handle
	LPCWSTR windowName = L"DX12Engine";		// Window name
	LPCWSTR windowTitle = L"DX12Engine";	// Window title

	// Size of the window
	int height = 720;
	int width = 1024;

	// Window properties
	bool fullscreen = false;

	bool InitWindow(HINSTANCE hInstance, int showWnd, int _width, int _height, bool fullscreen);

	void UpdateLoop();

	// Callback to handle windows messges
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Direct X 12 Setup
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

	ID3D12Device1* device;
	IDXGIAdapter1* adapter;
	IDXGIFactory4* dxgiFactory;
	IDXGISwapChain3* swapChain;
	int adapterIndex = 0;
	bool adapterFound = false;
	bool running = true;
};

