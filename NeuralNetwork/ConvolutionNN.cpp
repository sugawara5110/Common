//*****************************************************************************************//
//**                                                                                     **//
//**                              ConvolutionNN                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#include "ConvolutionNN.h"
#include <random>

void ConvolutionNN::SetLearningLate(float rate) {
	learningRate = rate;
}

ConvolutionNN::ConvolutionNN(unsigned int width, unsigned int height, unsigned int filNum, unsigned int elnumwid, unsigned int filstep) {

	elNumWid = elnumwid;
	Padding = elNumWid - 1;
	ElNum = elNumWid * elNumWid;
	filterStep = filstep;

	if (elNumWid % 2 == 0)MesB("filter要素数は奇数のみ有効です", elNumWid);
	if (elNumWid > 7)MesB("filter要素数はMax7です", elNumWid);
	if (filterStep > 8)MesB("filterスライド数はMax8です", filterStep);
	if (filterStep != 8 && filterStep != 4 &&
		filterStep != 2 && filterStep != 1)MesB("filterスライド数は2の累乗のみ有効です", filterStep);

	Width = width;
	Height = height;
	OutWid = Width / filterStep;
	OutHei = Height / filterStep;
	FilNum = filNum;
	fil = new float*[FilNum];
	for (unsigned int i = 0; i < FilNum; i++)fil[i] = new float[ElNum];
	ConvolutionIn = new float*[FilNum];
	ConvolutionOut = new float*[FilNum];
	outputError = new float*[FilNum];
	inputError = new float*[FilNum];
	for (unsigned int i = 0; i < FilNum; i++) {
		ConvolutionIn[i] = new float[(Width + Padding) * (Height + Padding)];
		ConvolutionOut[i] = new float[OutWid * OutHei];
		outputError[i] = new float[Width * Height];
		inputError[i] = new float[(OutWid + Padding) * (OutHei + Padding)];
	}
	for (unsigned int i = 0; i < (Width + Padding) * (Height + Padding); i++) {
		for (unsigned int j = 0; j < FilNum; j++) {
			ConvolutionIn[j][i] = 0.0f;
		}
	}
	for (unsigned int i = 0; i < (OutWid + Padding) * (OutHei + Padding); i++) {
		for (unsigned int j = 0; j < FilNum; j++) {
			inputError[j][i] = 0.0f;
		}
	}

	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	//フィルタ初期値
	std::normal_distribution<> dist(0.0, pow(ElNum, 0.2));

	for (unsigned int k = 0; k < FilNum; k++) {
		for (unsigned int i = 0; i < ElNum; i++) {
			fil[k][i] = dist(engine);
		}
	}
}

ConvolutionNN::~ConvolutionNN() {
	for (unsigned int i = 0; i < FilNum; i++) {
		ARRAY_DELETE(ConvolutionIn[i]);
		ARRAY_DELETE(ConvolutionOut[i]);
		ARRAY_DELETE(fil[i]);
		ARRAY_DELETE(outputError[i]);
		ARRAY_DELETE(inputError[i]);
	}
	ARRAY_DELETE(ConvolutionIn);
	ARRAY_DELETE(ConvolutionOut);
	ARRAY_DELETE(fil);
	ARRAY_DELETE(outputError);
	ARRAY_DELETE(inputError);
}

void ConvolutionNN::FirstInput(float el, unsigned int ElNum) {
	for (unsigned int i = 0; i < FilNum; i++)InputEl(el - 0.5f, i, ElNum);
}

void ConvolutionNN::InputEl(float el, unsigned int arrNum, unsigned int ElNum) {

	unsigned int y = ElNum / Width;
	unsigned int x = ElNum % Width;

	ConvolutionIn[arrNum][(Width + Padding) * (y + Padding / 2) + (x + Padding / 2)] = el;
}

void ConvolutionNN::Input(float *inArr, unsigned int arrNum) {
	for (unsigned int i = 0; i < Height; i++)
		memcpy(&ConvolutionIn[arrNum][(Width + Padding) * (i + Padding / 2) + Padding / 2], &inArr[Width * i], sizeof(float) * Width);
}

void ConvolutionNN::InputError(float *inArr, unsigned int arrNum) {
	for (unsigned int i = 0; i < OutHei; i++)
		memcpy(&inputError[arrNum][(OutWid + Padding) * (i + Padding / 2) + Padding / 2],
			&inArr[OutWid * i], sizeof(float) * OutWid);
}

void ConvolutionNN::ForwardPropagation() {

	for (unsigned int k = 0; k < FilNum; k++) {
		for (unsigned int j = 0; j < Height; j += filterStep) {
			for (unsigned int i = 0; i < Width; i += filterStep) {
				float tmp = 0;
				unsigned int cnt = 0;
				for (unsigned int j1 = j; j1 < elNumWid + j; j1++) {
					for (unsigned int i1 = i; i1 < elNumWid + i; i1++) {
						tmp += ConvolutionIn[k][(Width + Padding) * j1 + i1] * fil[k][cnt++];
					}
				}
				ConvolutionOut[k][OutWid * (j / filterStep) + (i / filterStep)] = Sigmoid(tmp);
			}
		}
	}
}

void ConvolutionNN::BackPropagation() {

	//下層から誤差伝搬
	for (unsigned int k = 0; k < FilNum; k++) {
		for (unsigned int i = 0; i < Width * Height; i++) {
			outputError[k][i] = 0.0f;
		}
	}
	for (unsigned int k = 0; k < FilNum; k++) {
		for (unsigned int j = 0; j < Height; j += filterStep) {
			unsigned int jo = j / filterStep;
			for (unsigned int i = 0; i < Width; i += filterStep) {
				unsigned int io = i / filterStep;
				//下層(pooling)からの誤差を要素毎に
				//該当するフィルター値を掛けoutputErrorに出力(上に層が無い場合使わないで終わる)
				unsigned int cnt = 0;
				for (unsigned int j1 = jo; j1 < elNumWid + jo; j1++) {
					for (unsigned int i1 = io; i1 < elNumWid + io; i1++) {
						outputError[k][Width * j + i] += inputError[k][(OutWid + Padding) * j1 + i1] * fil[k][cnt++];
					}
				}
			}
		}
	}

	//フィルタ更新
	for (unsigned int k = 0; k < FilNum; k++) {
		for (unsigned int j = 0; j < elNumWid; j++) {
			for (unsigned int i = 0; i < elNumWid; i++) {
				//elNumWid × elNumWidのフィルタの要素ごとに
				//下層(pooling層)から逆伝搬した誤差inputErrorと順伝搬時入力値の要素毎に乗算し全て足す
				//(計算時使用する順伝搬時入力値はフィルタ要素毎にずらして使用する。Padding領域も計算に含める形になる)
				float tmp = 0.0f;
				for (unsigned int h = j; h < Height + j; h += filterStep) {
					unsigned int ho = (h - j) / filterStep + j;
					for (unsigned int w = i; w < Width + i; w += filterStep) {
						unsigned int wo = (w - i) / filterStep + i;
						//要素毎に乗算した値を全て足す
						tmp += inputError[k][(OutWid + Padding) * (ho - j + Padding / 2) + (wo - i + Padding / 2)] *
							ConvolutionIn[k][(Width + Padding) * h + w];
					}
				}
				//上で足した値に学習率を乗算して現フィルタ値に加算する
				fil[k][elNumWid * j + i] += learningRate * tmp;
			}
		}
	}
}

float *ConvolutionNN::Output(unsigned int arrNum) {
	return ConvolutionOut[arrNum];
}

float ConvolutionNN::OutputEl(unsigned int arrNum, unsigned int ElNum) {
	return ConvolutionOut[arrNum][ElNum];
}

float ConvolutionNN::OutputFilter(unsigned int arrNum, unsigned int ElNum) {
	return fil[arrNum][ElNum];
}

float *ConvolutionNN::GetError(unsigned int arrNum) {
	return outputError[arrNum];
}

float ConvolutionNN::GetErrorEl(unsigned int arrNum, unsigned int ElNum) {
	return outputError[arrNum][ElNum];
}

void ConvolutionNN::SaveData(UINT Num) {
	FILE *fp;
	if (Num == 0)
		fp = fopen("save/save1.da", "wb");
	else
		fp = fopen("save/save2.da", "wb");
	float *tmp = new float[ElNum * FilNum];
	for (unsigned int k = 0; k < FilNum; k++) {
		for (unsigned int i = 0; i < ElNum; i++) {
			tmp[ElNum * k + i] = fil[k][i];
		}
	}
	fwrite(tmp, sizeof(float) * ElNum * FilNum, 1, fp);
	fclose(fp);
	ARRAY_DELETE(tmp);
}

void ConvolutionNN::LoadData(UINT Num) {
	FILE *fp;
	if (Num == 0)
		fp = fopen("save/save1.da", "rb");
	else
		fp = fopen("save/save2.da", "rb");
	float *tmp = new float[ElNum * FilNum];
	fread(tmp, sizeof(float) * ElNum * FilNum, 1, fp);
	for (unsigned int k = 0; k < FilNum; k++) {
		for (unsigned int i = 0; i < ElNum; i++) {
			fil[k][i] = tmp[ElNum * k + i];
		}
	}
	fclose(fp);
	ARRAY_DELETE(tmp);
}

unsigned int ConvolutionNN::GetOutWidth() {
	return OutWid;
}

unsigned int ConvolutionNN::GetOutHeight() {
	return OutHei;
}