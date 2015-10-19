#pragma once
#include "PaintAction.h"

BOOL CreateCanvasWindow(HINSTANCE hInst, HWND parentHWnd, int x, int y, int width, int height);
void OnMosueWheel(WPARAM wParam);
void Clear();
void Refresh();
void SetPaintTool(PaintTool tool);