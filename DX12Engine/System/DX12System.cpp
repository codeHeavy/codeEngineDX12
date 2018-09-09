
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
	width = _width;
	height = _height;
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
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // direct means the gpu can directly execute this command queue
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

	sampleDesc = {};
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
	//commandList->Close();	// Don't record command list now , recording done in the main loop

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
//	Setup the initial values for resources
//----------------------------------------------------------------------
bool DX12System::SetupResources()
{
	HRESULT hr;
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	if (FAILED(hr))
	{
		return false;
	}

	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	if (FAILED(hr))
	{
		return false;
	}

	// Creating shaders
	// Compile vertex shader
	ID3DBlob* vertexShader;		//	vertex shader bytecode
	ID3DBlob* errorBuffer;		// buffer to hold error data
	hr = D3DCompileFromFile(L"Shaders/VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &vertexShader, &errorBuffer);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
		return false;
	}

	// Shader bytecode structure
	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

	// Compile pixel shader
	ID3DBlob* pixelShader;
	hr = D3DCompileFromFile(L"Shaders/PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, &errorBuffer);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
		return false;
	}

	// pixel shader bytecode
	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();


	// Create Input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "COLOR" , 0, DXGI_FORMAT_R32G32B32A32_FLOAT,0,12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	// Create a pipeline stae object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = rootSignature;	// describes input data
	psoDesc.VS = vertexShaderBytecode;
	psoDesc.PS = pixelShaderBytecode;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	// render target format
	psoDesc.SampleDesc = sampleDesc;	// must be same as swapchain and depth stencil
	psoDesc.SampleMask = 0xffffffff;	// 0xffffffff = point sampling
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);	// default rasterizer state
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.NumRenderTargets = 1;

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
	if (FAILED(hr))
	{
		return false;
	}
	// Create vertex buffer

	// Triangle vertices
	Vertex vertexList[] = {
		{ 0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	};

	int vertexBufferSize = sizeof(vertexList);

	// Create default heap
	// Default heap = memory in GPU
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),	// default heap
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
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
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,					// GPU will read from this and copy to default heap
		nullptr,
		IID_PPV_ARGS(&vertexBufferUploadHeap)
		);
	vertexBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	// Store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast<BYTE*>(vertexList);	// pointer to vertex array
	vertexData.RowPitch = vertexBufferSize;					// size of all triangle data
	vertexData.SlicePitch = vertexBufferSize;				// size of triangle data

	// Command from command list to copy data from upload heap to default heap
	UpdateSubresources(commandList, vertexBuffer, vertexBufferUploadHeap, 0, 0, 1, &vertexData);

	// transition vertex buffer data from copy destination satte to vertex buffer state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// Execute command list to upload initial assets
	commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// increment the fence value
	fenceValue[frameIndex]++;
	hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(hr))
	{
		running = false;
	}

	// create a vertex buffer view for the triangle
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = vertexBufferSize;

	// Viewport
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Scissor Rect
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = width;
	scissorRect.bottom = height;

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

	hr = commandList->Reset(commandAllocator[frameIndex], pipelineStateObject);
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

	// Draw call
	Draw();

	// transition render target from render target state to curtrrent state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	hr = commandList->Close();	// resets to recording state
	if (FAILED(hr))
	{
		running = false;
	}
}

//----------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------
void DX12System::Draw()
{
	// draw triangle
	commandList->SetGraphicsRootSignature(rootSignature);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->DrawInstanced(3, 1, 0, 0);
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
	
	SAFE_RELEASE(pipelineStateObject);
	SAFE_RELEASE(rootSignature);
	SAFE_RELEASE(vertexBuffer);
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