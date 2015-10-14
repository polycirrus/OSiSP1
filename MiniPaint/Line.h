#pragma once
#include "Shape.h"
class Line :
	public Shape
{
public:
	int x1, y1, x2, y2;

	Line();
	Line(int x1, int y1, int x2, int y2);
	~Line();

	void Draw(HDC hdc, int xOffset, int yOffset);
};

