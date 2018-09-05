#include "stdafx.h"
#include "System/DX12System.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance, LPSTR lpCmdLIne, int nCmdShow)
{
	if (!DX12System::GetInstance()->InitWindow(hInstance, nCmdShow, 1024, 720, false))
	{
		MessageBox(0, L"Window Initialization Falied", L"Error", MB_OK);
		return 0;
	}

	DX12System::GetInstance()->UpdateLoop();
	return 0;
}