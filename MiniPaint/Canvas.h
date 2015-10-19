#pragma once
#include "PaintAction.h"

BOOL CreateCanvasWindow(HINSTANCE hInst, HWND parentHWnd, int x, int y, int width, int height);
void OnMouseWheel(WPARAM wParam);
void Clear();
void Refresh();
void SetPaintTool(PaintTool tool);
void SetPenColor(COLORREF color);
COLORREF GetPenColor();
BOOL ResizeWindow(int x, int y, int width, int height);