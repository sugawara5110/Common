//*****************************************************************************************//
//**                                                                                     **//
//**                              ConvolutionNN                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Common.h"

class ConvolutionNN {

private:
	unsigned int Width; //入力画像サイズ
	unsigned int Height;//入力画像サイズ
	unsigned int OutWid;//出力画像サイズ
	unsigned int OutHei;//出力画像サイズ

	unsigned int elNumWid = 3;//奇数のみMax7
	unsigned int Padding = 2; //elNumWid-1
	unsigned int ElNum = 9;  //elNumWid * elNumWid
	unsigned int filterStep = 1;//2の累乗のみ,Max8まで

	//畳込みフィルター
	float **fil = nullptr;
	unsigned int FilNum = 1;
	//畳込み前
	float **ConvolutionIn = nullptr;
	//畳込み後
	float **ConvolutionOut = nullptr;
	//下層からの誤差
	float **inputError = nullptr;
	//下層からの誤差ウエイト計算後上層に送る時に使用
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