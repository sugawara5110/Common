//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　NeuralNetwork                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "NeuralNetwork.h"
#include <random>

NeuralNetwork::NeuralNetwork(unsigned int *numNode, int depth, unsigned int split) {

	Split = split;
	NumNode = numNode;
	NumNode[0] *= Split;
	if (depth > 5)Depth = 5;
	else
		Depth = depth;
	total = new float*[Depth];
	totalSig = new float*[Depth];
	error = new float*[Depth];
	weight = new MATRIX_NN[Depth - 1];
	weightTmp = new MATRIX_NN[Depth - 1];

	for (int i = 0; i < Depth; i++) {
		total[i] = new float[NumNode[i]];
		totalSig[i] = new float[NumNode[i]];
		error[i] = new float[NumNode[i]];
	}

	output = new float[NumNode[Depth - 1]];
	target = new float[NumNode[Depth - 1]];

	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	for (int i = 0; i < 5; i++) {
		if (NumNode[i] < 1)NumNode[i] = 1;
	}
	//ウエイト初期値
	// 平均0.0、標準偏差pow(NumNode[], -0.5)で分布させる
	std::normal_distribution<> dist0(0.0, pow(NumNode[1], -0.5));
	std::normal_distribution<> dist1(0.0, pow(NumNode[2], -0.5));
	std::normal_distribution<> dist2(0.0, pow(NumNode[3], -0.5));
	std::normal_distribution<> dist3(0.0, pow(NumNode[4], -0.5));

	float *wei = nullptr;
	float *wei2 = nullptr;
	for (int k = 0; k < Depth - 1; k++) {
		unsigned int wNum = NumNode[k] * NumNode[k + 1];
		wei = new float[wNum];
		wei2 = new float[wNum];
		weightNum += wNum;
		for (unsigned int i = 0; i < NumNode[k + 1]; i++) {
			for (unsigned int j = 0; j < NumNode[k]; j++) {
				double rnd;
				switch (k) {
				case 0:
					rnd = dist0(engine);
					break;
				case 1:
					rnd = dist1(engine);
					break;
				case 2:
					rnd = dist2(engine);
					break;
				case 3:
					rnd = dist3(engine);
					break;
				}
				wei[NumNode[k] * i + j] = (float)rnd;
			}
		}
		weight[k].Input(wei, NumNode[k], NumNode[k + 1]);//(X:そのノード数, Y:次のノード数)
		weightTmp[k].Input(wei2, NumNode[k], NumNode[k + 1]);
		wei = nullptr;
		wei2 = nullptr;
	}
}

NeuralNetwork::~NeuralNetwork() {

	ARRAY_DELETE(output);
	ARRAY_DELETE(target);
	for (int i = 0; i < Depth; i++) {
		ARRAY_DELETE(total[i]);
		ARRAY_DELETE(totalSig[i]);
		ARRAY_DELETE(error[i]);
	}
	ARRAY_DELETE(total);
	ARRAY_DELETE(totalSig);
	ARRAY_DELETE(error);

	ARRAY_DELETE(weight);
	ARRAY_DELETE(weightTmp);
}

void NeuralNetwork::SetLearningLate(float rate) {
	learningRate = rate;
}

void NeuralNetwork::SetTarget(float *tar) {
	memcpy(target, tar, sizeof(float) * NumNode[Depth - 1]);
}

void NeuralNetwork::SetTargetEl(float el, unsigned int ElNum) {
	target[ElNum] = el;
}

void NeuralNetwork::FirstInput(float el, unsigned int ElNum) {
	for (unsigned int i = 0; i < Split; i++)InputArrayEl(el, i, ElNum);
}

void NeuralNetwork::InputArray(float *inArr, unsigned int arrNum) {
	memcpy(&totalSig[0][(NumNode[0] / Split) * arrNum], inArr, sizeof(float) * (NumNode[0] / Split));
}

void NeuralNetwork::InputArrayEl(float el, unsigned int arrNum, unsigned int ElNum) {
	totalSig[0][(NumNode[0] / Split) * arrNum + ElNum] = el;
}

void NeuralNetwork::GetOutput(float *out) {
	for (unsigned int i = 0; i < NumNode[Depth - 1]; i++) {
		out[i] = output[i];
	}
}

float NeuralNetwork::GetOutputEl(unsigned int ElNum) {
	return output[ElNum];
}

void NeuralNetwork::ForwardPropagation() {

	for (int k = 0; k < Depth - 1; k++) {
		MatrixVectorDot(total[k + 1], &weight[k], totalSig[k]);
		SigmoidArr(totalSig[k + 1], total[k + 1], NumNode[k + 1]);
	}
	for (unsigned int i = 0; i < NumNode[Depth - 1]; i++)output[i] = totalSig[Depth - 1][i];
}

void NeuralNetwork::BackPropagation() {

	for (unsigned int i = 0; i < NumNode[Depth - 1]; i++)
		error[Depth - 1][i] = target[i] - totalSig[Depth - 1][i];

	for (int k = Depth - 2; k >= 0; k--) {
		weight[k].MatrixTransposeUpdate();
		weight[k].Transpose(true);
		MatrixVectorDot(error[k], &weight[k], error[k + 1]);
		weight[k].Transpose(false);
	}

	for (unsigned int k = Depth - 1; k >= 1; k--) {
		for (unsigned int j = 0; j < NumNode[k]; j++) {
			total[k][j] = error[k][j] * totalSig[k][j] * (1.0f - totalSig[k][j]);
		}
		VectorDot(weightTmp[k - 1].GetMatrix(), total[k], NumNode[k], totalSig[k - 1], NumNode[k - 1]);
		unsigned int mX = weight[k - 1].GetX();
		unsigned int mY = weight[k - 1].GetY();
		for (unsigned int j = 0; j < mY; j++) {
			for (unsigned int i = 0; i < mX; i++) {
				float el = weight[k - 1].GetMatrix()[mX * j + i] + weightTmp[k - 1].GetMatrix()[mX * j + i] * learningRate;
				weight[k - 1].InputElement(el, i, j);
			}
		}
	}
}

void NeuralNetwork::Query() {
	ForwardPropagation();
}

void NeuralNetwork::Training() {
	ForwardPropagation();
	BackPropagation();
}

float *NeuralNetwork::GetError(unsigned int arrNum) {
	return &error[0][(NumNode[0] / Split) * arrNum];
}

float NeuralNetwork::GetErrorEl(unsigned int arrNum, unsigned int ElNum) {
	return error[0][(NumNode[0] / Split) * arrNum + ElNum];
}

void NeuralNetwork::InverseQuery() {

	for (int k = Depth - 2; k >= 0; k--) {
		for (unsigned int i = 0; i < NumNode[k]; i++) {
			totalSig[k][i] = 0.0f;
			for (unsigned int j = 0; j < NumNode[k + 1]; j++) {
				if (k == Depth - 2)
					totalSig[k][i] += Logit(target[j]) * weight[k].GetMatrix()[NumNode[k + 1] * i + j];
				else
					totalSig[k][i] += Logit(totalSig[k + 1][j]) * weight[k].GetMatrix()[NumNode[k + 1] * i + j];
			}
			total[k][i] = Logit(totalSig[k][i]);
		}
	}
}

float *NeuralNetwork::GetInverseOutput(unsigned int arrNum) {
	return &total[0][(NumNode[0] / Split) * arrNum];
}

float NeuralNetwork::GetInverseOutputEl(unsigned int arrNum, unsigned int ElNum) {
	return total[0][(NumNode[0] / Split) * arrNum + ElNum];
}

void NeuralNetwork::SaveData() {
	FILE *fp = fopen("save/save.da", "wb");
	float *weightArr = new float[weightNum];

	unsigned int cnt = 0;
	for (int k = 0; k < Depth - 1; k++) {
		for (unsigned int i = 0; i < NumNode[k]; i++) {
			for (unsigned int j = 0; j < NumNode[k + 1]; j++) {
				weightArr[cnt++] = weight[k].GetMatrix()[NumNode[k + 1] * i + j];
			}
		}
	}

	fwrite(weightArr, sizeof(float) * weightNum, 1, fp);
	fclose(fp);
	ARRAY_DELETE(weightArr);
}

void NeuralNetwork::LoadData() {
	FILE *fp = fopen("save/save.da", "rb");
	float *weightArr = new float[weightNum];
	fread(weightArr, sizeof(float) * weightNum, 1, fp);
	fclose(fp);

	unsigned int cnt = 0;
	for (int k = 0; k < Depth - 1; k++) {
		for (unsigned int i = 0; i < NumNode[k]; i++) {
			for (unsigned int j = 0; j < NumNode[k + 1]; j++) {
				weight[k].GetMatrix()[NumNode[k + 1] * i + j] = weightArr[cnt++];
			}
		}
	}
	ARRAY_DELETE(weightArr);
}
