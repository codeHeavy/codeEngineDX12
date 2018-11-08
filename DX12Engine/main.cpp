#include "stdafx.h"
#include "System/DX12System.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance, LPSTR lpCmdLIne, int nCmdShow)
{
#if _DEBUG
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	if (!DX12System::GetInstance()->InitWindow(hInstance, nCmdShow, 1024, 720, false))
	{
		MessageBox(0, L"Window Initialization Falied", L"Error", MB_OK);
		return 0;
	}

	if (!DX12System::GetInstance()->InitD3D())
	{
		MessageBox(0, L"Direct3D Initialization Falied", L"Error", MB_OK);
		DX12System::GetInstance()->Cleanup();
		return 1;
	}

	if (!DX12System::GetInstance()->SetupResources())
	{
		MessageBox(0, L"Resource Initialization Falied", L"Error", MB_OK);
		DX12System::GetInstance()->Cleanup();
		return 1;
	}

	DX12System::GetInstance()->UpdateLoop();
	DX12System::GetInstance()->Cleanup();
	return 0;
}