#include "stdafx.h"
#include "MetafilePlayback.h"
#include <math.h>


MetafilePlayback::MetafilePlayback(HENHMETAFILE handle)
{
	hFile = handle;
}


MetafilePlayback::~MetafilePlayback()
{
}

void MetafilePlayback::Draw(HDC hdc, int xOffset, int yOffset)
{
	int iWidthMM = GetDeviceCaps(hdc, HORZSIZE);
	int iHeightMM = GetDeviceCaps(hdc, VERTSIZE);
	int iWidthPels = GetDeviceCaps(hdc, HORZRES);
	int iHeightPels = GetDeviceCaps(hdc, VERTRES);

	ENHMETAHEADER metafileHeader;
	GetEnhMetaFileHeader(hFile, sizeof(ENHMETAHEADER), &metafileHeader);

	RECT boundingRect;
	boundingRect.left = 0;
	boundingRect.top = 0;
	boundingRect.right = round(metafileHeader.rclFrame.right / (double)iWidthMM * (double)iWidthPels / 100);
	boundingRect.bottom = round(metafileHeader.rclFrame.bottom / (double)iHeightMM * (double)iHeightPels / 100);

	PlayEnhMetaFile(hdc, hFile, &boundingRect);
}