#include "stdafx.h"
#include "Canvas.h"

HINSTANCE hInstance;
HWND parentWindow, thisWindow;
LPCWSTR thisClassName = L"canvas", thisWindowName = L"Canvas";
bool isWindowCreated;

ATOM				RegisterCanvasAreaClass();
LRESULT CALLBACK	CanvasWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL CreateCanvasWindow(HINSTANCE hInst, HWND parentHWnd, int x, int y, int width, int height)
{
	if (isWindowCreated)
		return FALSE;

	hInstance = hInst;
	parentWindow = parentHWnd;

	thisWindow = CreateWindow(
		thisClassName, 
		thisWindowName,
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL,
		x, 
		y, 
		width, 
		height, 
		parentHWnd, 
		nullptr, 
		hInst, 
		nullptr);

	if (!thisWindow)
		return FALSE;

	isWindowCreated = true;
	UpdateWindow(thisWindow);

	return TRUE;
}

ATOM RegisterCanvasAreaClass()
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = CanvasWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = thisClassName;
	wcex.hIconSm = nullptr;

	return RegisterClassExW(&wcex);
}

LRESULT CALLBACK CanvasWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}