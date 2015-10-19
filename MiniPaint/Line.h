#pragma once
#include "PaintAction.h"

class Line :
	public PaintAction
{
public:
	int x1, y1, x2, y2;

	Line();
	Line(int x1, int y1);
	~Line();

	void Draw(HDC hdc, int xOffset, int yOffset);
};

