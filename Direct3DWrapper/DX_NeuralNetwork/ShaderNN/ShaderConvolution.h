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
"RWStructuredBuffer<float> gGradient : register(u7);\n"//���z
"RWStructuredBuffer<float> gGradBias : register(u8);\n"//bias���z

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gWidHei;\n"//����w:x, ����h:y ,FilNum:z
"    float4 gfilWid_filStep;\n"//Filwid��:x, Filstep��:y
"    float4 gLear_inputS;\n"//���g�p:x, inputset��:y, ���g�p:z
"};\n"

//Dispatch(APP��)(X1, Y1, Z1)numthreads(CS��)(X, Y, Z)
//x,y,z,x1,y1,z1 �́`�Ԗ�
//X,Y,Z,X1,Y1,Z1 ��Max�l
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint ���̑�uint3

//zero������
"[numthreads(1, 1, 1)]\n"
"void CNInitCS(uint3 inid : SV_DispatchThreadID)\n"
"{\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * inid.z;\n"
"   gInput[InsetInd + gWidHei.x * inid.y + inid.x] = 0.0f;\n"
"}\n"

//input�g��
//input������
"[numthreads(?**, ?**, 1)]\n"
"void CNFPCS0(uint3 inid : SV_DispatchThreadID)\n"
"{\n"
"   uint inwid = gWidHei.x;\n"//input��wid��
"   uint inhei = gWidHei.y;\n"//input��hei��
"   uint ix = inid.x;\n"//input, widIndex
"   uint iy = inid.y % inhei;\n"//input, heiIndex(filter�܂܂Ȃ�)
"   uint numInd = inid.y / inhei;\n"
"   uint inStInd = numInd * inwid * inhei;\n"//input, Filter���P�ʂ�index * �v�f��
"   uint InsetInd = inwid * inhei * gWidHei.z * inid.z;\n"

"   uint outwid = gWidHei.x * gfilWid_filStep.y;\n"//output��wid��
"   uint outhei = gWidHei.y * gfilWid_filStep.y;\n"//output��hei��
"   uint ox = ix * gfilWid_filStep.y;\n"
"   uint oy = iy * gfilWid_filStep.y;\n"
"   uint outStInd = numInd * outwid * outhei;\n"//output, Filter���P�ʂ�index * �v�f��
"   uint OutsetInd = outwid * outhei * gWidHei.z * inid.z;\n"

"   uint inInd = InsetInd + inStInd + inwid * iy + ix;\n"
"   uint outInd = OutsetInd + outStInd + outwid * oy + ox;\n"

"   gOutput[outInd] = gInput[inInd];\n"
"}\n"

//���`�d
//�o�͑�����񏈗�,���͑������[�v(�X���b�h���͏o�͑��Ɠ���)
"[numthreads(?**, ?**, 1)]\n"//�ő�X * Y * Z = 1024
"void CNFPCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   int outwid = gWidHei.x / gfilWid_filStep.y;\n"//�o��wid��
"   int outhei = gWidHei.y / gfilWid_filStep.y;\n"//�o��hei��
"   uint setInd = outid.z;\n"//���X���b�h
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"//���͑����X���b�hST�ʒu
"   uint OutsetInd = outwid * outhei * gWidHei.z * setInd;\n"//�o�͑����X���b�hST�ʒu
"   int ox = outid.x;\n"//�o��widIndex
"   int oy = outid.y % outhei;\n"//�o��heiIndex(fil����)
"   int ix = ox * gfilWid_filStep.y;\n"//����widIndex
"   int iy = oy * gfilWid_filStep.y;\n"//����heiIndex
"   int padding = gfilWid_filStep.x / 2;\n"//�t�B���^�[wid/2�̒l��padding���Ƃ���
"   uint filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"//Filter�v�f��
"   uint numInd = outid.y / outhei;\n"//�X���b�hY����Filter���P�ʂ�index
"   uint filStInd = numInd * filElNum;\n"//Filter�z���Filter���P�ʂ�index * �v�f��
"   uint inStInd = numInd * gWidHei.x * gWidHei.y;\n"//Input�z���Filter���P�ʂ�index * �v�f��
"   uint outStInd = numInd * outwid * outhei;\n"//Output�z���Filter���P�ʂ�index * �v�f��
"   uint FilNumInd = filStInd / filElNum;\n"//��Filter��index(�v�findex�ł͂Ȃ�)

"   float tmp = 0.0f;\n"
"   for(uint i = 0; i < filElNum; i++)\n"
"   {\n"
"      int fx = (i % (int)gfilWid_filStep.x) - padding;\n"//Filter���WX - padding
"      int fy = (i / (int)gfilWid_filStep.x) - padding;\n"//Filter���WY - padding
"      if(iy + fy >= 0 && iy + fy < gWidHei.y && ix + fx >= 0 && ix + fx < gWidHei.x)\n"//Padding�̈�̓X�L�b�v
"      {\n"
"         tmp += gInput[InsetInd + inStInd + gWidHei.x * (iy + fy) + (ix + fx)] * gFilter[filStInd + i] * \n"
"                gDropOutF[inStInd + gWidHei.x * (iy + fy) + (ix + fx)] + gBias[FilNumInd];\n"
"      }\n"
"   }\n"
"   gOutput[OutsetInd + outStInd + outwid * oy + ox] = tmp;\n"
"}\n"

//inErr�g��
//inErr������
"[numthreads(?**, ?**, 1)]\n"
"void CNBPCS0(uint3 inEid : SV_DispatchThreadID)\n"
"{\n"
"   uint inEwid = gWidHei.x / gfilWid_filStep.y;\n"//inErr��wid��
"   uint inEhei = gWidHei.y / gfilWid_filStep.y;\n"//inErr��hei��
"   uint iEx = inEid.x;\n"//inErr, widIndex
"   uint iEy = inEid.y % inEhei;\n"//inErr, heiIndex(filter�܂܂Ȃ�)
"   uint numInd = inEid.y / inEhei;\n"
"   uint inEStInd = numInd * inEwid * inEhei;\n"//inErr, Filter���P�ʂ�index * �v�f��
"   uint InEsetInd = inEwid * inEhei * gWidHei.z * inEid.z;\n"

"   uint outEwid = gWidHei.x;\n"//outErr��wid��
"   uint outEhei = gWidHei.y;\n"//outErr��hei��
"   uint oEx = iEx * gfilWid_filStep.y;\n"//outErr, widIndex
"   uint oEy = iEy * gfilWid_filStep.y;\n"//outErr, heiIndex(filter�܂܂Ȃ�)
"   uint outEStInd = numInd * outEwid * outEhei;\n"//outErr, Filter���P�ʂ�index * �v�f��
"   uint OutEsetInd = outEwid * outEhei * gWidHei.z * inEid.z;\n"

"   uint inErrInd = InEsetInd + inEStInd + inEwid * iEy + iEx;\n"
"   uint outErrInd = OutEsetInd + outEStInd + outEwid * oEy + oEx;\n"

"   gOutErr[outErrInd] = gInErr[inErrInd];\n"
"}\n"

//�t�`�d
//Err�o�͑�����񏈗�,Err���͑������[�v(�X���b�h���͓��͐��Ɠ���)
"[numthreads(?**, ?**, 1)]\n"//�ő�X * Y * Z = 1024   
"void CNBPCS1(uint3 outEid : SV_DispatchThreadID)\n"
"{\n"
"   int inEwid = gWidHei.x * gfilWid_filStep.y;\n"//inErr��wid��
"   int inEhei = gWidHei.y * gfilWid_filStep.y;\n"//inErr��hei��
"   int iEx = outEid.x * gfilWid_filStep.y;\n"//inErr, widIndex
"   int iEy = (outEid.y * (int)gfilWid_filStep.y) % inEhei;\n"//inErr, heiIndex(filter�܂܂Ȃ�)
"   int oEx = iEx / gfilWid_filStep.y;\n"//outErr, widIndex
"   int oEy = iEy / gfilWid_filStep.y;\n"//outErr, heiIndex(filter�܂܂Ȃ�)
"   int padding = gfilWid_filStep.x / 2;\n"
"   int filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"//filter�v�f��
"   int numInd = outEid.y / gWidHei.y;\n"
"   int filStInd = numInd * filElNum;\n"
"   int inEStInd = numInd * inEwid * inEhei;\n"//inErr, Filter���P�ʂ�index * �v�f��
"   int outEStInd = numInd * gWidHei.x * gWidHei.y;\n"//outErr, Filter���P�ʂ�index * �v�f��
"   uint setInd = outEid.z;\n"
"   uint OutEsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   uint InEsetInd = inEwid * inEhei * gWidHei.z * setInd;\n"

"   float tmp = 0.0f;\n"
"   for(int i = filElNum - 1; i >= 0; i--)\n"
"   {\n"
"      int fx = (i % (uint)gfilWid_filStep.x) - padding;\n"
"      int fy = (i / (uint)gfilWid_filStep.x) - padding;\n"
"      if(iEy + fy >= 0 && iEy + fy < inEhei && iEx + fx >= 0 && iEx + fx < inEwid)\n"//Padding�̈�̓X�L�b�v
"      {\n"
"         uint inErrInd = InEsetInd + inEStInd + inEwid * (iEy + fy) + (iEx + fx);\n"
"         tmp += gInErr[inErrInd] * gFilter[filStInd + i] * gDropOutF[inEStInd + inEwid * (iEy + fy) + (iEx + fx)];\n"
"      }\n"
"   }\n"
"   gOutErr[OutEsetInd + outEStInd + gWidHei.x * oEy + oEx] = tmp;\n"
"}\n"

//�t�B���^���z�X�V
//�t�B���^����񏈗�(�X���b�h���̓t�B���^�[�v�f�� * �t�B���^�[���Ɠ���)
"[numthreads(?**, ?**, 1)]\n"//�ő�X * Y * Z = 1024   
"void CNBPCS2(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   int filx = inid.x;\n"//Filter�v�fXIndex
"   int fily = inid.y % gfilWid_filStep.x;\n"//Filter�v�fYIndex(Filter���܂܂Ȃ�)
"   int padding = gfilWid_filStep.x / 2;\n"
"   uint inErrwid = gWidHei.x * gfilWid_filStep.y;\n"
"   uint inErrhei = gWidHei.y * gfilWid_filStep.y;\n"
"   uint numInd = inid.y / gfilWid_filStep.x;\n"
"   uint filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"
"   uint filStInd = numInd * filElNum;\n"
"   uint inEStInd = numInd * inErrwid * inErrhei;\n"
"   uint inStInd = numInd * gWidHei.x * gWidHei.y;\n"

"   float tmpSum = 0.0f;\n"
"   for(int k = 0; k < gLear_inputS.y; k++)\n"//inputSet����loop
"   {\n"
"      uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * k;\n"
"      uint OutsetInd = inErrwid * inErrhei * gWidHei.z * k;\n"
"      float tmp = 0.0f;\n"
"      for(uint i = 0; i < inErrwid * inErrhei; i+=gfilWid_filStep.y)\n"
"      {\n"
"         int Ex = i % inErrwid;\n"//inerrX
"         int Ey = i / inErrwid;\n"//inerrY
"         int Ix = Ex / (uint)gfilWid_filStep.y + (filx - padding);\n"//inputX
"         int Iy = Ey / (uint)gfilWid_filStep.y + (fily - padding);\n"//inputY

"         if(Ix >= 0 && Ix < gWidHei.x && Iy >= 0 && Iy < gWidHei.y)\n"
"         {\n"
"            float inE = gInErr[OutsetInd + inEStInd + inErrwid * Ey + Ex];\n"
"            tmp += (inE * gInput[InsetInd + inStInd + gWidHei.x * Iy + Ix]);\n"
"         }\n"
"      }\n"
"      tmpSum += tmp;\n"
"   }\n"

"   uint grInd = filStInd + gfilWid_filStep.x * fily + filx;\n"
"   gGradient[grInd] = tmpSum / gLear_inputS.y;\n"
"}\n"

//bias���z�X�V
//Filter���ɕ��񏈗�
"[numthreads(?**, 1, 1)]\n"//�ő�X * Y * Z = 1024  
"void CNBPCSBias(int2 filid : SV_DispatchThreadID)\n"
"{\n"
"   int inwid = gWidHei.x * gfilWid_filStep.y;\n"//���w����̌덷wid
"   int inhei = gWidHei.y * gfilWid_filStep.y;\n"
"   int errNum = inwid * inhei;\n"

"   float tmp = 0.0f;\n"
"   for(int k = 0; k < gLear_inputS.y; k++)\n"
"   {\n"
"      for(int i = 0; i < errNum; i++)\n"
"      {\n"
"         tmp += gInErr[errNum * gWidHei.z * k + errNum * filid.x + i];\n"
"      }\n"
"   }\n"
"   gGradBias[filid.x] = tmp / gLear_inputS.y;\n"
"}\n";


