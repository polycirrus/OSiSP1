#pragma once
#include "PaintAction.h"
class MetafilePlayback :
	public PaintAction
{
public:
	MetafilePlayback(HENHMETAFILE handle);
	~MetafilePlayback();
	void Draw(HDC hdc, int xOffset, int yOffset);
private:
	HENHMETAFILE hFile;
};

