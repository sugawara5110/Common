//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　　　　  Sound_クラス                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Sound_Header
#define Class_Sound_Header

#include "DsProcess.h"

class Sound_ :public DsProcess {

public:
	Sound_() {}
	Sound_(char* filename);
	void sound(bool repeat, long volume);//volume -10000～0
	void soundoff();
	void soundloop(bool repeat, long volume, REFTIME start, REFTIME end);
};

#endif
