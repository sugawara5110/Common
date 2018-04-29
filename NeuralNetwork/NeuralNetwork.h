//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　NeuralNetwork                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Common.h"

class MATRIX_NN {

private:
	float *m[2] = { nullptr };
	unsigned int X[2] = { 0 };
	unsigned int Y[2] = { 0 };
	unsigned int ind = 0;

public:
	~MATRIX_NN();
	void Input(float *in, unsigned int x, unsigned int y);
	void InputElement(float e, unsigned int x, unsigned int y);
	float *GetMatrix();
	unsigned int GetX() { return X[ind]; }
	unsigned int GetY() { return Y[ind]; }
	void MatrixTransposeUpdate();
	void Transpose(bool f);
};

void MatrixVectorDot(float *OutVec, MATRIX_NN *mat, float *vec);
void VectorDot(float *OutMat, float *ColVec, unsigned int colNum, float *RowVec, unsigned int rowNum);

class NeuralNetwork final {

private:
	float **total = nullptr;
	float **totalSig = nullptr;
	float **error = nullptr;
	float *output = nullptr;
	unsigned int Split = 1;

	MATRIX_NN *weight = nullptr;
	MATRIX_NN *weightTmp = nullptr;

	float *target;
	float learningRate = 0.1f;

	unsigned int weightNum = 0;

	unsigned int *NumNode = nullptr;//各ノードの数
	int Depth;

	NeuralNetwork() {}
	void ForwardPropagation();
	void BackPropagation();

public:
	NeuralNetwork(unsigned int *numNode, int depth, unsigned int split);
	~NeuralNetwork();
	void SetLearningLate(float rate);
	void SetTarget(float *target);
	void SetTargetEl(float el, unsigned int ElNum);
	void FirstInput(float el, unsigned int ElNum);
	void InputArray(float *inArr, unsigned int arrNum);
	void InputArrayEl(float el, unsigned int arrNum, unsigned int ElNum);
	void GetOutput(float *out);
	float GetOutputEl(unsigned int ElNum);
	void Query();
	void Training();
	float *GetError(unsigned int arrNum);
	float GetErrorEl(unsigned int arrNum, unsigned int ElNum);
	void InverseQuery();
	float *GetInverseOutput(unsigned int arrNum);
	float GetInverseOutputEl(unsigned int arrNum, unsigned int ElNum);
	void SaveData();
	void LoadData();
};