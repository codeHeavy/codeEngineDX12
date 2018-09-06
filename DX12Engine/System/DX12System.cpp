
#include "DX12System.h"

DX12System* DX12System::dxInstance = 0;

DX12System* DX12System::GetInstance()
{
	if (!dxInstance)
	{
		dxInstance = new DX12System();
	}
	return dxInstance;
}

DX12System::DX12System()
{
	
}


DX12System::~DX12System()
{
}

// Initialize window
bool DX12System::InitWindow(HINSTANCE hInstance, int showWnd, int _width, int _height, bool fullscreen)
{
	if (fullscreen)
	{
		HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monInfo = { sizeof(monInfo)};

		width = monInfo.rcMonitor.right - monInfo.rcMonitor.left;
		height = monInfo.rcMonitor.bottom - monInfo.rcMonitor.top;
	}

	WNDCLASSEX wndClass = {};

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;;
	wndClass.lpfnWndProc = DX12System::WndProc;
	wndClass.cbClsExtra = NULL;
	wndClass.cbWndExtra = NULL;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = windowName;
	wndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wndClass))
	{
		MessageBox(NULL, L"Error registering class", L"Error",MB_OK | MB_ICONERROR);
		return false;
	}

	hwnd = CreateWindowEx(NULL, windowName, windowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, hInstance, NULL);

	if (!hwnd)
	{
		MessageBox(NULL, L"Error creating window", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (fullscreen)
	{
		SetWindowLong(hwnd, GWL_STYLE, 0);
	}

	ShowWindow(hwnd, showWnd);
	UpdateWindow(hwnd);

	return true;
}

// Window update loop
void DX12System::UpdateLoop()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{ 
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Game code
			Update();
			Render();
		}
	}

	WaitForPreviousFrame();
	CloseHandle(fenceEvent);
}

// Initialize Direct 3D
bool DX12System::InitD3D()
{
	HRESULT hr;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr))
	{
		return false;
	}
	// Find first GPU that supports DX 12
	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapterIndex++;
			continue;
		}

		// Direct 12 compatible device
		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
			adapterFound = true;
			break;
		}

		adapterIndex++;
	}

	if (!adapterFound)
	{
		return false;
	}

	// Create the device
	hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
	if (FAILED(hr))
	{
		return false;
	}

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));	// Create command queue
	if (FAILED(hr))
	{
		return false;
	}

	//Create SwapChain

DXGI_MODE_DESC backBufferDesc = {};
backBufferDesc.Width = width;							// buffer width
backBufferDesc.Height = height;							// buffer height
backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;		// buffer format (rgba 32bit, 8bit for each channel)

DXGI_SAMPLE_DESC sampleDesc = {};
sampleDesc.Count = 1;									// Not multisampling so count is 1

DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
swapChainDesc.BufferCount = BUFFERCOUNT;
swapChainDesc.BufferDesc = backBufferDesc;
swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// pipeline will render to this swapchain
swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
swapChainDesc.OutputWindow = hwnd;
swapChainDesc.SampleDesc = sampleDesc;
swapChainDesc.Windowed = !fullscreen;

IDXGISwapChain* tempSwapChain;

dxgiFactory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);

swapChain = static_cast<IDXGISwapChain3*> (tempSwapChain);			// IDXGISwapChian3 has GetCurrentBackBuffer method
frameIndex = swapChain->GetCurrentBackBufferIndex();

// Create back buffers (RTVs) discriptor heap
D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
rtvHeapDesc.NumDescriptors = BUFFERCOUNT;
rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// non shader visible 
hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescrioptorHeap));
if (FAILED(hr))
{
	return false;
}

rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	// get the size of the RTV descriptor size

CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescrioptorHeap->GetCPUDescriptorHandleForHeapStart());	// get the first descriptor in the heap

// Create and rtv for each buffer
for (int i = 0; i < BUFFERCOUNT; i++)
{
	hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
	if (FAILED(hr)) {
		return false;
	}

	// create the render target which binds the swap chain to rtv
	device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);

	// increment rtv handle by the descriptor size
	rtvHandle.Offset(1, rtvDescriptorSize);
}

// Command Allocators - allocate memory on the GPU for commads
for (int i = 0; i < BUFFERCOUNT; i++)
{
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]));
	if (FAILED(hr))
	{
		return false;
	}
}

// Command List - # equals the number of threads
hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[0], NULL, IID_PPV_ARGS(&commandList));
if (FAILED(hr))
{
	return false;
}
commandList->Close();	// Don't record command list now , recording done in the main loop

// Fences and Fence Event
// 1 Thread = one fence event
// tripple buffering = 3 fences for each frame buffer

// Create Fences
for (int i = 0; i < BUFFERCOUNT; i++)
{
	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i]));
	if (FAILED(hr))
	{
		return false;
	}

	fenceValue[i] = 0;	// initial value 0
}

// fence event handle
fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
if (fenceEvent == nullptr)
{
	return false;
}

return true;
}

//----------------------------------------------------------------------
//	Update
//----------------------------------------------------------------------
void DX12System::Update()
{

}

//----------------------------------------------------------------------
// Update Pipeline
// add commands to command list
// change state of render target, set root signature,clear render target
// set vertex buffer, draw call
//----------------------------------------------------------------------
void DX12System::UpdatePipeline()
{
	HRESULT hr;
	
	// Wait for GPU to finish with command allocator
	WaitForPreviousFrame();

	// reset allocator when GPU is done
	hr = commandAllocator[frameIndex]->Reset();
	if (FAILED(hr))
	{
		running = false;
	}

	hr = commandList->Reset(commandAllocator[frameIndex], NULL);
	if (FAILED(hr))
	{
		running = false;
	}

	// transition render target from current ot render target state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// get handle of current RTV to set as render target in OM stage
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescrioptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

	// set render target for OM stage
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Clear render target
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// transition render target from render target state to curtrrent state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	hr = commandList->Close();	// resets to recording state
	if (FAILED(hr))
	{
		running = false;
	}
}

//----------------------------------------------------------------------
// Render
//----------------------------------------------------------------------
void DX12System::Render()
{
	HRESULT hr;

	UpdatePipeline();	//	send commands to the command queue

	ID3D12CommandList* _commandLists[] = { commandList };	// create array of command lists

	commandQueue->ExecuteCommandLists(_countof(_commandLists), _commandLists);	// execute command list

	// chaeck when command queue completes execution
	hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(hr))
	{
		running = false;
	}

	hr = swapChain->Present(0, 0);
	if (FAILED(hr))
	{
		running = false;
	}
}


//----------------------------------------------------------------------
// Cleanup
//----------------------------------------------------------------------
void DX12System::Cleanup()
{
	// Wait for GPU to finish all frames
	for (int i = 0; i < frameBufferCount; i++)
	{
		frameIndex = 1;
		WaitForPreviousFrame();
	}

	BOOL fs = false;
	if (swapChain->GetFullscreenState(&fs, NULL))
		swapChain->SetFullscreenState(false, NULL);
	
	SAFE_RELEASE(device);
	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(commandQueue);
	SAFE_RELEASE(rtvDescrioptorHeap);
	SAFE_RELEASE(commandList);

	for (int i = 0; i < BUFFERCOUNT; ++i)
	{
		SAFE_RELEASE(renderTargets[i]);
		SAFE_RELEASE(commandAllocator[i]);
		SAFE_RELEASE(fence[i]);
	}
	
}

//----------------------------------------------------------------------
// Wait for GPU to finish previours frame
//----------------------------------------------------------------------
void DX12System::WaitForPreviousFrame()
{
	HRESULT hr;

	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// if current fence value < "fenceValue" ===> GPU still in execution
	if (fence[frameIndex]->GetCompletedValue() < fenceValue[frameIndex])
	{
		hr = fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent);
		if (FAILED(hr))
		{
			running = false;
		}

		// Wait till fence has triggered
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// Increment fence value for next frame
	fenceValue[frameIndex]++;
}

//----------------------------------------------------------------------
// Windows messages callback
//----------------------------------------------------------------------
LRESULT DX12System::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
		switch (msg)
		{

		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE) {
				if (MessageBox(0, L"Are you sure you want to exit?",
					L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
					DestroyWindow(hWnd);
			}
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
}