
#include "DX12System.h"
//
int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBuffer) + 255) & ~255;
DX12System* DX12System::dxInstance = 0;

LRESULT DX12System::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return dxInstance->WndProc(hWnd, uMsg, wParam, lParam);
}

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
	wndClass.lpfnWndProc = DX12System::WindowProc;
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
	__int64 now;
	QueryPerformanceCounter((LARGE_INTEGER*)&now);
	startTime = now;
	currentTime = now;
	previousTime = now;

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
			UpdateTimer();
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
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[frameIndex], NULL, IID_PPV_ARGS(&commandList));
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
	fpsFrameCount = 0;
	fpsTimeElapsed = 0.0f;

	__int64 perfFreq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&perfFreq);
	perfCounterSeconds = 1.0 / (double)perfFreq;

	DirectX::ResourceUploadBatch resourceUpload(device);
	deferredRenderer = new DefferedRenderer(device, width, height);
	deferredRenderer->Init(commandList);
	HRESULT hr;

	// descriptor range table = range of descriptors inside the descriptor heap
	D3D12_DESCRIPTOR_RANGE descriptorTableRange[1];
	descriptorTableRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorTableRange[0].NumDescriptors = 1;
	descriptorTableRange[0].BaseShaderRegister = 0;
	descriptorTableRange[0].RegisterSpace = 0;
	descriptorTableRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//create descriptor table
	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = _countof(descriptorTableRange);
	descriptorTable.pDescriptorRanges = &descriptorTableRange[0];

	//	create a root descriptor 
	D3D12_ROOT_DESCRIPTOR	rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;

	// create root parameter
	D3D12_ROOT_PARAMETER rootParameters[3];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor = rootCBVDescriptor;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable = descriptorTable;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[2].Descriptor = rootCBVDescriptor;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Create static sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters),
		rootParameters,
		1,	// static sampler
		&sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // deny shader stages here for better performance
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

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
		{ "TEXCOORD" , 0, DXGI_FORMAT_R32G32_FLOAT,0,12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
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
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
	if (FAILED(hr))
	{
		return false;
	}
	
	resourceUpload.Begin();
	CreateWICTextureFromFile(device, resourceUpload, L"Assets/Images/MetalPlate/Metal_Plate_010_baseColor_A.jpg", &textureBuffer);
	
	texture = new Texture(L"Assets/Images/MetalPlate/Metal_Plate_010_baseColor_A.jpg",device, commandList);
	normalTexture = new Texture(L"Assets/Images/MetalPlate/Metal_Plate_010_normal.jpg", device, commandList);
	roughnessTexture = new Texture(L"Assets/Images/MetalPlate/Metal_Plate_010_roughness_A.jpg", device, commandList);
	metalTexture = new Texture(L"Assets/Images/MetalPlate/Metal_Plate_010_metallic_A.jpg", device, commandList);
	deferredRenderer->SetSRV(textureBuffer, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	deferredRenderer->SetSRV(normalTexture->GetTexture(), normalTexture->GetFormat(), 1);
	deferredRenderer->SetSRV(roughnessTexture->GetTexture(), roughnessTexture->GetFormat(), 2);
	deferredRenderer->SetSRV(metalTexture->GetTexture(), metalTexture->GetFormat(), 3);
	
	auto uploadOperation = resourceUpload.End(commandQueue);
	uploadOperation.wait();
	
	mesh = new Mesh("Assets/Models/sphere.obj", device, commandList);
	// Create depth stencil
	// Create depth stencil descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap));
	if (FAILED(hr))
	{
		running = false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearVlaue = {};
	depthOptimizedClearVlaue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearVlaue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearVlaue.DepthStencil.Stencil = 0;

	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearVlaue,
		IID_PPV_ARGS(&depthStencilBuffer)
	);
	dsDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");
	device->CreateDepthStencilView(depthStencilBuffer, &depthStencilDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Set lights
	PSCBuffer.light.AmbientColor = XMFLOAT4(0.1, 0.1, 0.1, 1.0);
	PSCBuffer.light.DiffuseColor = XMFLOAT4(1, 0, 0, 1);
	PSCBuffer.light.Direction = XMFLOAT3(1, -1, 0);

	PSCBuffer.pLight.Color = XMFLOAT4(0, 0, 1, 1);
	PSCBuffer.pLight.Position = XMFLOAT3(0.5, 0.5, 0.0);

	// create constant buffer resource heap
	// Resource heaps must be multiples of 64KB of size

	for (int i = 0; i < BUFFERCOUNT; i++)
	{
		hr = device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), // resource heap must be multiples of 64KB for single textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constantBufferUploadHeap[i])
		);
		constantBufferUploadHeap[i]->SetName(L"Constant Buffer Upload Resource Heap");
		ZeroMemory(&constantBufferPerObject, sizeof(constantBufferPerObject));
		CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)

		hr = constantBufferUploadHeap[i]->Map(0, &readRange, reinterpret_cast<void**>(&constantBufferGPUAddress[i]));

		// constant buffers must be 256 bytes aligned
		memcpy(constantBufferGPUAddress[i], &constantBufferPerObject, sizeof(constantBufferPerObject));
		memcpy(constantBufferGPUAddress[i] + ConstantBufferPerObjectAlignedSize, &constantBufferPerObject, sizeof(constantBufferPerObject));
		memcpy(constantBufferGPUAddress[i] + ConstantBufferPerObjectAlignedSize * 2, &PSCBuffer, sizeof(PSCBuffer));
	}

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

	// clear memory of image
//	delete imageData;

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

	BuildViewProjMatrix();

	return true;
}

//----------------------------------------------------------------------
//	Build the view and projection matrix
//----------------------------------------------------------------------
void DX12System::BuildViewProjMatrix()
{

	camera = new Camera();
	camera->Update();
	camera->UpdateProjectionMatrix(width, height);
	PSCBuffer.CamPos = DirectX::XMFLOAT4(camera->GetPosition().x, camera->GetPosition().y, camera->GetPosition().z, 1.0);
	// first cube
	cube1 = new GameObject(mesh);
	cube1->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
	cube1->SetScale(XMFLOAT3(2, 2, 2));
	
	// second cube
	cube2 = new GameObject(mesh);
	cube2->SetPosition(XMFLOAT3(0.5f, 0.0f, 0.0f));
	
	cube1->UpdateWorldMatrix();
	cube2->UpdateWorldMatrix();
}

//----------------------------------------------------------------------
//	Update
//----------------------------------------------------------------------
void DX12System::Update()
{
	camera->Update(deltaTime);
	cube1->UpdateWorldMatrix();
	cube2->UpdateWorldMatrix();
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

	// get handle to depth stencil buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// set render target for OM stage
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Clear render target
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, FALSE, nullptr);

	// Draw call
	Draw();
	commandList->OMSetRenderTargets(1, &rtvHandle, true, nullptr);
	commandList->ClearRenderTargetView(rtvHandle, clearColor, FALSE, nullptr);

	deferredRenderer->ApplyLightingPSO(commandList,true,PSCBuffer);
	deferredRenderer->DrawLightPass(commandList);
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
	// clear depth stencil buffer
	commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	//--------------------Deferred Rendering-----------------------

	deferredRenderer->ApplyGBufferPSO(commandList,true, cube1, camera,PSCBuffer);
	deferredRenderer->Render(commandList);
	
	deferredRenderer->ApplyLightingShapePSO(commandList, true,PSCBuffer);
	deferredRenderer->RenderLightShape(commandList,PSCBuffer);
	//--------------------Deferred Rendering-----------------------
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
	//SAFE_RELEASE(vertexBuffer);
	SAFE_RELEASE(depthStencilBuffer);
	SAFE_RELEASE(dsDescriptorHeap);

	for (int i = 0; i < frameBufferCount; ++i)
	{
		//SAFE_RELEASE(mainDescriptorHeap[i]);
		SAFE_RELEASE(constantBufferUploadHeap[i]);
	};

	delete texture;
	delete normalTexture;
	delete roughnessTexture;
	delete metalTexture;
	delete deferredRenderer;
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

void DX12System::OnMouseDown(WPARAM buttonState, int x, int y)
{
	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;

	// Caputure the mouse so we keep getting mouse move
	// events even if the mouse leaves the window.  we'll be
	// releasing the capture once a mouse button is released
	SetCapture(hwnd);
}
void DX12System::OnMouseUp(WPARAM buttonState, int x, int y)
{
	// We don't care about the tracking the cursor outside
	// the window anymore (we're not dragging if the mouse is up)
	ReleaseCapture();
}
void DX12System::OnMouseMove(WPARAM buttonState, int x, int y)
{
	// Check left mouse button
	if (buttonState & 0x0001)
	{
		float xDiff = (x - prevMousePos.x) * 0.005f;
		float yDiff = (y - prevMousePos.y) * 0.005f;
		camera->Rotate(xDiff, yDiff);
	}

	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;
}
void DX12System::OnMouseWheel(float wheelDelta, int x, int y)
{

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
			// Mouse button being pressed (while the cursor is currently over our window)
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			OnMouseDown( wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

			// Mouse button being released (while the cursor is currently over our window)
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			OnMouseUp( wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

			// Cursor moves over the window (or outside, while we're currently capturing it)
		case WM_MOUSEMOVE:
			OnMouseMove( wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

			// Mouse wheel is scrolled
		case WM_MOUSEWHEEL:
			OnMouseWheel( GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		}
		return DefWindowProc( hWnd, msg, wParam, lParam);
}



void DX12System::UpdateTimer()
{
	__int64 now;
	QueryPerformanceCounter((LARGE_INTEGER*)&now);
	currentTime = now;

	deltaTime = max((float)((currentTime - previousTime) * perfCounterSeconds), 0.0f);
	totalTime = (float)((currentTime - startTime) * perfCounterSeconds);
	previousTime = currentTime;
}