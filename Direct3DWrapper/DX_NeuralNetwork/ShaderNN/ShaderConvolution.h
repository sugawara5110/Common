///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                    ShaderConvolution.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderConvolution =
"RWStructuredBuffer<float> gInput : register(u0);\n"
"RWStructuredBuffer<float> gOutput : register(u1);\n"
"RWStructuredBuffer<float> gInErr : register(u2);\n"
"RWStructuredBuffer<float> gOutErr : register(u3);\n"
"RWStructuredBuffer<float> gFilter : register(u4);\n"
"RWStructuredBuffer<float> gDropOutF : register(u5);\n"
"RWStructuredBuffer<float> gBias : register(u6);\n"

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gWidHei;\n"//MaxFilNum:z
"    float4 gfilWid_filStep;\n"
"    float4 gLear_inputS;\n"//�w�K��:x, inputset��:y, bias�w�K��:z
"};\n"

//Dispatch(APP��)(X1, Y1, Z1)numthreads(CS��)(X, Y, Z)
//x,y,z,x1,y1,z1 �́`�Ԗ�
//X,Y,Z,X1,Y1,Z1 ��Max�l
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint ���̑�uint3

"#define FP_X ?**\n"
"#define FP_Y ?**\n"
//���`�dsigmoid
//�o�͑�����񏈗�,���͑������[�v(�X���b�h���͏o�͑��Ɠ���)
"[numthreads(FP_X, FP_Y, 1)]\n"//�ő�X * Y * Z = 1024
"void CNFPCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   int outwid = gWidHei.x / gfilWid_filStep.y;\n"
"   int outhei = gWidHei.y / gfilWid_filStep.y;\n"
"   uint setInd = outid.z;\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   uint OutsetInd = outwid * outhei * gWidHei.z * setInd;\n"
"   int ox = outid.x;\n"
"   int oy = outid.y % outhei;\n"
"   int ix = ox * gfilWid_filStep.y;\n"
"   int iy = oy * gfilWid_filStep.y;\n"
"   int padding = gfilWid_filStep.x / 2;\n"
"   uint filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"
"   uint numInd = outid.y * gfilWid_filStep.y / gWidHei.y;\n"
"   uint filStInd = numInd * filElNum;\n"
"   uint inStInd = numInd * gWidHei.x * gWidHei.y;\n"
"   uint outStInd = numInd * outwid * outhei;\n"
"   uint FilNumInd = filStInd / filElNum;\n"//��filter��index

"   float tmp = 0.0f;\n"
"   for(uint i = 0; i < filElNum; i++)\n"
"   {\n"
"      int fx = (i % gfilWid_filStep.x) - padding;\n"
"      int fy = (i / gfilWid_filStep.x) - padding;\n"
"      if(iy + fy >= 0 && iy + fy < gWidHei.y && ix + fx >= 0 && ix + fx < gWidHei.x)\n"//Padding�̈�̓X�L�b�v
"      {\n"
"         tmp += gInput[InsetInd + inStInd + gWidHei.x * (iy + fy) + (ix + fx)] * gFilter[filStInd + i] * \n"
"                gDropOutF[inStInd + gWidHei.x * (iy + fy) + (ix + fx)] + gBias[FilNumInd];\n"
"      }\n"
"   }\n"
"   float sig = 1.0f / (1.0f + pow(2.71828182846, -tmp));\n"
"   gOutput[OutsetInd + outStInd + outwid * oy + ox] = sig;\n"
"}\n"

//���`�dReLU
//�o�͑�����񏈗�,���͑������[�v(�X���b�h���͏o�͑��Ɠ���)
"[numthreads(FP_X, FP_Y, 1)]\n"//�ő�X * Y * Z = 1024,  (47�s)
"void CNFPReLUCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   int outwid = gWidHei.x / gfilWid_filStep.y;\n"
"   int outhei = gWidHei.y / gfilWid_filStep.y;\n"
"   uint setInd = outid.z;\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   uint OutsetInd = outwid * outhei * gWidHei.z * setInd;\n"
"   int ox = outid.x;\n"
"   int oy = outid.y % outhei;\n"
"   int ix = ox * gfilWid_filStep.y;\n"
"   int iy = oy * gfilWid_filStep.y;\n"
"   int padding = gfilWid_filStep.x / 2;\n"
"   uint filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"
"   uint numInd = outid.y * gfilWid_filStep.y / gWidHei.y;\n"
"   uint filStInd = numInd * filElNum;\n"
"   uint inStInd = numInd * gWidHei.x * gWidHei.y;\n"
"   uint outStInd = numInd * outwid * outhei;\n"
"   uint FilNumInd = filStInd / filElNum;\n"//��filter��index

"   float tmp = 0.0f;\n"
"   for(uint i = 0; i < filElNum; i++)\n"
"   {\n"
"      int fx = (i % gfilWid_filStep.x) - padding;\n"
"      int fy = (i / gfilWid_filStep.x) - padding;\n"
"      if(iy + fy >= 0 && iy + fy < gWidHei.y && ix + fx >= 0 && ix + fx < gWidHei.x)\n"//Padding�̈�̓X�L�b�v
"      {\n"
"         tmp += gInput[InsetInd + inStInd + gWidHei.x * (iy + fy) + (ix + fx)] * gFilter[filStInd + i] * \n"
"                gDropOutF[inStInd + gWidHei.x * (iy + fy) + (ix + fx)] + gBias[FilNumInd];\n"
"      }\n"
"   }\n"
"   float relu = max(0, tmp);\n"
"   gOutput[OutsetInd + outStInd + outwid * oy + ox] = relu;\n"
"}\n"

//gOutErr������
"[numthreads(?**, ?**, 1)]\n"//�ő�X * Y * Z = 1024  (80�s)
"void CNBPCS0(int3 inid : SV_DispatchThreadID)\n"
"{\n"
"   int x = inid.x;\n"
"   int y = inid.y;\n"
"   uint setInd = inid.z;\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   gOutErr[InsetInd + gWidHei.x * y + x] = 0.0f;\n"
"}\n"

//�t�`�d
//Err�o�͑�����񏈗�,Err���͑������[�v(�X���b�h���͓��͐��Ɠ���)
"[numthreads(?**, ?**, 1)]\n"//�ő�X * Y * Z = 1024   (106�s)
"void CNBPCS1(uint3 inid : SV_DispatchThreadID)\n"
"{\n"
"   int inwid = gWidHei.x / gfilWid_filStep.y;\n"
"   int inhei = gWidHei.y / gfilWid_filStep.y;\n"
"   int ix = inid.x;\n"
"   int iy = inid.y % inhei;\n"
"   int ox = ix * gfilWid_filStep.y;\n"
"   int oy = iy * gfilWid_filStep.y;\n"
"   int padding = gfilWid_filStep.x / 2;\n"
"   int filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"
"   int numInd = inid.y * gfilWid_filStep.y / gWidHei.y;\n"
"   int filStInd = numInd * filElNum;\n"
"   int inStInd = numInd * inwid * inhei;\n"
"   int outStInd = numInd * gWidHei.x * gWidHei.y;\n"
"   uint setInd = inid.z;\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   uint OutsetInd = inwid * inhei * gWidHei.z * setInd;\n"

"   float tmp = 0.0f;\n"
"   for(int i = filElNum - 1; i >= 0; i--)\n"
"   {\n"
"      int fx = (i % gfilWid_filStep.x) - padding;\n"
"      int fy = (i / gfilWid_filStep.x) - padding;\n"
"      if(iy + fy >= 0 && iy + fy < inhei && ix + fx >= 0 && ix + fx < inwid)\n"//Padding�̈�̓X�L�b�v
"      {\n"
"         tmp += gInErr[OutsetInd + inStInd + inwid * (iy + fy) + (ix + fx)] * gFilter[filStInd + i];\n"
"      }\n"
"   }\n"
"   gOutErr[InsetInd + outStInd + gWidHei.x * oy + ox] = tmp * gDropOutF[outStInd + gWidHei.x * oy + ox];\n"
"}\n"

"#define BP_X ?**\n"
"#define BP_Y ?**\n"
//�t�B���^�X�Vsigmoid
//�t�B���^����񏈗�(�X���b�h���̓t�B���^�[�v�f�� * �t�B���^�[���Ɠ���)
"[numthreads(BP_X, BP_Y, 1)]\n"//�ő�X * Y * Z = 1024   (136�s)
"void CNBPCS2(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   int filx = inid.x;\n"
"   int fily = inid.y % gfilWid_filStep.x;\n"
"   int padding = gfilWid_filStep.x / 2;\n"
"   uint inErrwid = gWidHei.x / gfilWid_filStep.y;\n"
"   uint inErrhei = gWidHei.y / gfilWid_filStep.y;\n"
"   uint numInd = inid.y / gfilWid_filStep.x;\n"
"   uint filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"
"   uint filStInd = numInd * filElNum;\n"
"   uint inEStInd = numInd * inErrwid * inErrhei;\n"
"   uint inStInd = numInd * gWidHei.x * gWidHei.y;\n"

"   float tmpSum = 0.0f;\n"
"   for(int k = 0; k < gLear_inputS.y; k++)\n"
"   {\n"
"      uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * k;\n"
"      uint OutsetInd = inErrwid * inErrhei * gWidHei.z * k;\n"
"      float tmp = 0.0f;\n"
"      for(uint i = 0; i < inErrwid * inErrhei; i+=gfilWid_filStep.y)\n"
"      {\n"
"         int Ex = i % inErrwid;\n"
"         int Ey = i / inErrwid;\n"
"         int Ix = Ex * gfilWid_filStep.y + (filx - padding);\n"
"         int Iy = Ey * gfilWid_filStep.y + (fily - padding);\n"

"         if(Ix >= 0 && Ix < gWidHei.x && Iy >= 0 && Iy < gWidHei.y)\n"
"         {\n"
"            tmp += gInErr[OutsetInd + inEStInd + inErrwid * Ey + Ex] * \n"
"                   gInput[InsetInd + inStInd + gWidHei.x * Iy + Ix];\n"
"         }\n"
"      }\n"
"      tmpSum += tmp;\n"
"   }\n"
"   float tmpAve = tmpSum / gLear_inputS.y;\n"
"   gFilter[filStInd + gfilWid_filStep.x * fily + filx] -= tmpAve * gLear_inputS.x;\n"
"}\n"

//�t�B���^�X�VReLU
//�t�B���^����񏈗�(�X���b�h���̓t�B���^�[�v�f�� * �t�B���^�[���Ɠ���)
"[numthreads(BP_X, BP_Y, 1)]\n"//�ő�X * Y * Z = 1024   (172�s)
"void CNBPReLUCS2(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   int filx = inid.x;\n"
"   int fily = inid.y % gfilWid_filStep.x;\n"
"   int padding = gfilWid_filStep.x / 2;\n"
"   uint inErrwid = gWidHei.x / gfilWid_filStep.y;\n"
"   uint inErrhei = gWidHei.y / gfilWid_filStep.y;\n"
"   uint numInd = inid.y / gfilWid_filStep.x;\n"
"   uint filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"
"   uint filStInd = numInd * filElNum;\n"
"   uint inEStInd = numInd * inErrwid * inErrhei;\n"
"   uint inStInd = numInd * gWidHei.x * gWidHei.y;\n"

"   float tmpSum = 0.0f;\n"
"   for(int k = 0; k < gLear_inputS.y; k++)\n"
"   {\n"
"      uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * k;\n"
"      uint OutsetInd = inErrwid * inErrhei * gWidHei.z * k;\n"
"      float tmp = 0.0f;\n"
"      for(uint i = 0; i < inErrwid * inErrhei; i+=gfilWid_filStep.y)\n"
"      {\n"
"         int Ex = i % inErrwid;\n"
"         int Ey = i / inErrwid;\n"
"         int Ix = Ex * gfilWid_filStep.y + (filx - padding);\n"
"         int Iy = Ey * gfilWid_filStep.y + (fily - padding);\n"

"         if(Ix >= 0 && Ix < gWidHei.x && Iy >= 0 && Iy < gWidHei.y)\n"
"         {\n"
"            float inE = gInErr[OutsetInd + inEStInd + inErrwid * Ey + Ex];\n"
"            float outEl = gOutput[OutsetInd + inEStInd + inErrwid * Ey + Ex];\n"
"            if(outEl > 0.0f)tmp += (inE * gInput[InsetInd + inStInd + gWidHei.x * Iy + Ix]);\n"
"         }\n"
"      }\n"
"      tmpSum += tmp;\n"
"   }\n"
"   float tmpAve = tmpSum / gLear_inputS.y;\n"
"   gFilter[filStInd + gfilWid_filStep.x * fily + filx] -= tmpAve * gLear_inputS.x;\n"
"}\n"

//bias�X�V
//Filter���ɕ��񏈗�
"[numthreads(?**, 1, 1)]\n"//�ő�X * Y * Z = 1024  (89�s)
"void CNBPCSBias(int2 filid : SV_DispatchThreadID)\n"
"{\n"
"   int inwid = gWidHei.x / gfilWid_filStep.y;\n"//���w����̌덷wid
"   int inhei = gWidHei.y / gfilWid_filStep.y;\n"
"   int errNum = inwid * inhei;\n"

"   float tmp = 0.0f;\n"
"   for(int k = 0; k < gLear_inputS.y; k++)\n"
"   {\n"
"      for(int i = 0; i < errNum; i++)\n"
"      {\n"
"         tmp += gInErr[errNum * gWidHei.z * k + errNum * filid.x + i];\n"
"      }\n"
"   }\n"
"   float ave = tmp / gLear_inputS.y;\n"
"   gBias[filid.x] -= ave * gLear_inputS.z;\n"
"}\n";


