#pragma once
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
	HWND hwnd = NULL;					// Windows handle
	LPCWSTR windowName = L"DX12Engine";	// Window name
	LPCWSTR windowTitle = L"DX12Engine"; // Window title

	// Size of the window
	int height = 720;
	int width = 1024;

	// Window properties
	bool fullscreen = false;

	bool InitWindow(HINSTANCE hInstance, int showWnd, int _width, int _height, bool fullscreen);

	void UpdateLoop();

	// Callback to handle windows messges
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

