//*****************************************************************************************//
//**                                                                                     **//
//**                              ConvolutionNN                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Common.h"

class ConvolutionNN {

private:
	unsigned int Width; //���͉摜�T�C�Y
	unsigned int Height;//���͉摜�T�C�Y
	unsigned int OutWid;//�o�͉摜�T�C�Y
	unsigned int OutHei;//�o�͉摜�T�C�Y

	unsigned int elNumWid = 3;//��̂�Max7
	unsigned int Padding = 2; //elNumWid-1
	unsigned int ElNum = 9;  //elNumWid * elNumWid
	unsigned int filterStep = 1;//2�̗ݏ�̂�,Max8�܂�

	//�􍞂݃t�B���^�[
	float **fil = nullptr;
	unsigned int FilNum = 1;
	//�􍞂ݑO
	float **ConvolutionIn = nullptr;
	//�􍞂݌�
	float **ConvolutionOut = nullptr;
	//���w����̌덷
	float **inputError = nullptr;
	//���w����̌덷�E�G�C�g�v�Z���w�ɑ��鎞�Ɏg�p
	float **outputError = nullptr;

	float learningRate = 0.1f;

	ConvolutionNN() {}

public:
	ConvolutionNN(unsigned int width, unsigned int height, unsigned int filNum, unsigned int elnumwid = 3, unsigned int filstep = 1);
	~ConvolutionNN();
	void SetLearningLate(float rate);
	void FirstInput(float el, unsigned int ElNum);
	void Input(float *inArr, unsigned int arrNum);
	void InputEl(float el, unsigned int arrNum, unsigned int ElNum);
	void InputError(float *inArr, unsigned int arrNum);
	void ForwardPropagation();
	void BackPropagation();
	float *Output(unsigned int arrNum);
	float OutputEl(unsigned int arrNum, unsigned int ElNum);
	float OutputFilter(unsigned int arrNum, unsigned int ElNum);
	float *GetError(unsigned int arrNum);
	float GetErrorEl(unsigned int arrNum, unsigned int ElNum);
	unsigned int GetOutWidth();
	unsigned int GetOutHeight();
	void SaveData(UINT Nnm);
	void LoadData(UINT Num);
};