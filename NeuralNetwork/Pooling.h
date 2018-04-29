//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　  　　Pooling                                               **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Common.h"

#define POOL 2

class Pooling {

private:
	unsigned int Width;
	unsigned int Height;
	unsigned int OutWid;
	unsigned int OutHei;
	unsigned int OddNumWid = 0;
	unsigned int OddNumHei = 0;
	float **input = nullptr;
	float **output = nullptr;
	unsigned int PoolNum = 1;
	float **inerror = nullptr;
	float **outerror = nullptr;

	Pooling(){}

public:
	Pooling(unsigned int width, unsigned int height, unsigned int poolNum);
	~Pooling();
	void FirstInput(float el, unsigned int ElNum);
	void Input(float *inArr, unsigned int arrNum);
	void InputEl(float el, unsigned int arrNum, unsigned int ElNum);
	void InputError(float *inArr, unsigned int arrNum);
	void InputErrorEl(float el, unsigned int arrNum, unsigned int ElNum);
	void ForwardPropagation();
	void BackPropagation();
	float *Output(unsigned int arrNum);
	float OutputEl(unsigned int arrNum, unsigned int ElNum);
	float *GetError(unsigned int arrNum);
	float GetErrorEl(unsigned int arrNum, unsigned int ElNum);
	unsigned int GetOutWidth();
	unsigned int GetOutHeight();
};
