//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　　　　  Cameraクラス                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Camera_Header
#define Class_Camera_Header

#include "Movie.h"

class Camera :public Movie {

private:
	ICreateDevEnum *pCreateDevEnum = NULL;
	IEnumMoniker *pEnumMoniker = NULL;
	IMoniker *pMoniker = NULL;
	ULONG nFetched = 0;

public:
	Camera();
	UINT **GetFrame(int width, int height);
	BYTE *GetFrame1(int width, int height);
};

#endif
