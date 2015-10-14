#pragma once
class Shape
{
public:
	virtual void Draw(HDC hdc, int xOffset, int yOffset) = 0;
};
