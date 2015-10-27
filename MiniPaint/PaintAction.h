#pragma once
class PaintAction
{
public:
	virtual void Draw(HDC hdc, int xOffset, int yOffset) = 0;
};

enum class PaintTool
{
	None,
	Pencil,
	Line,
	Polyline,
	Rectangle
};