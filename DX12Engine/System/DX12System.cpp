#include "stdafx.h"
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
		}
	}
}

// Windows messages callback
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

