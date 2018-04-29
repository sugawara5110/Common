///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        ShaderNeuralNetwork.hlsl                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderNeuralNetwork =
"RWStructuredBuffer<float> gInNode : register(u0);\n"
"RWStructuredBuffer<float> gOutNode : register(u1);\n"
"RWStructuredBuffer<float> gWeight : register(u2);\n"
"RWStructuredBuffer<float> gInError : register(u3);\n"
"RWStructuredBuffer<float> gOutError : register(u4);\n"

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gLear_Depth;\n"//�w�K��:x, �������[��:y, MaxDepth:z
"    float4 gNumNode[5];\n"//�e�w��Node��:x, gNode,gError�e�w�J�n�C���f�b�N�X:y
"    float4 gNumWeight[4];\n"//gWeight�e�w�J�n�C���f�b�N�X:x
"    float4 gTarget[10];\n"//target�l:x
"};\n"

//Dispatch(APP��)(X1, Y1, Z1)numthreads(CS��)(X, Y, Z)
//x,y,z,x1,y1,z1 �́`�Ԗ�
//X,Y,Z,X1,Y1,Z1 ��Max�l
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint ���̑�uint3

//���`��
//�o�͑�����񏈗�,���͑������[�v gLear_Depth.y = 0�`Depth-2�܂�
"[numthreads(1, 1, 1)]\n"//�ő�X * Y * Z = 1024
"void NNFPCS(int3 outid : SV_DispatchThreadID)\n"
"{\n"
"   float WeightStartInd = gNumWeight[gLear_Depth.y].x;\n"
"   float InNodeNum = gNumNode[gLear_Depth.y].x;\n"
"   float OutNodeNum = gNumNode[gLear_Depth.y + 1].x;\n"
"   int detecInd = outid.z;\n"

"   float tmp = 0.0f;\n"
"   int x = outid.x;\n"
"   for(int i = 0; i < InNodeNum; i++)\n"
"   {\n"
"      tmp += gInNode[InNodeNum * detecInd + i] * gWeight[WeightStartInd + InNodeNum * x + i];\n"
"   }\n"
"   float sig = 1.0f / (1.0f + pow(2.71828182846, -tmp));\n"
"   gOutNode[OutNodeNum * detecInd + x] = sig;\n"
"}\n"

//�t�`���OTarget�l���� gLear_Depth.y = Depth-1�̂�
"[numthreads(1, 1, 1)]\n"//�ő�X * Y * Z = 1024
"void InTargetCS(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   int x = inid.x;\n"
"   gInError[x] = gTarget[x].x - gInNode[x];\n"
"}\n"

//�t�`��
//���͑�����񏈗�,�o�͑������[�v gLear_Depth.y = Depth-2�`0�܂�
"[numthreads(1, 1, 1)]\n"//�ő�X * Y * Z = 1024
"void NNBPCS0(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   float WeightStartInd = gNumWeight[gLear_Depth.y].x;\n"
"   float InNodeNum = gNumNode[gLear_Depth.y].x;\n"
"   float OutNodeNum = gNumNode[gLear_Depth.y + 1].x;\n"

"   float tmp = 0.0f;\n"
"   int x = inid.x;\n"
"   for(int i = 0; i < OutNodeNum; i++)\n"
"   {\n"
"      tmp += gOutError[i] * gWeight[WeightStartInd + InNodeNum * i + x];\n"
"   }\n"
"   gInError[x] = tmp;\n"
"}\n"

//weight�l�X�V
//gWeight����X�V gLear_Depth.y = Depth-2�`0�܂�
"[numthreads(1, 1, 1)]\n"//�ő�X * Y * Z = 1024
"void NNBPCS1(int2 inXoutY : SV_DispatchThreadID)\n"
"{\n"
"   float WeightStartInd = gNumWeight[gLear_Depth.y].x;\n"

"   int x = inXoutY.x;\n"//In��
"   int y = inXoutY.y;\n"//Out��
"   float tmp = 0.0f;\n"
"   tmp = gOutError[y] * gOutNode[y] * (1.0f - gOutNode[y]);\n"
"   tmp = tmp * gInNode[x] * gLear_Depth.x;\n"
"   int w = gNumNode[gLear_Depth.y].x * y + x;\n"
"   gWeight[WeightStartInd + w] += tmp;\n"
"}\n"

//�t����
//���͑�����񏈗�,�o�͑������[�v gLear_Depth.y = Depth-1�`1�܂�
"[numthreads(1, 1, 1)]\n"//�ő�X * Y * Z = 1024
"void NNInverseCS(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   float WeightStartInd = gNumWeight[gLear_Depth.y - 1].x;\n"
"   float InNodeNum = gNumNode[gLear_Depth.y - 1].x;\n"
"   float OutNodeNum = gNumNode[gLear_Depth.y].x;\n"

"   float tmp = 0.0f;\n"
"   int x = inid.x;\n"
"   for(int i = 0; i < OutNodeNum; i++)\n"
"   {\n"
"      float logit;\n"
"      if(gLear_Depth.y == gLear_Depth.z)logit = gTarget[i].x;\n"
"      else logit = gOutNode[i];\n"

"      if (logit < 0.01f)logit = 0.01f;\n"
"      if (logit > 0.99f)logit = 0.99f;\n"
"      logit = log(logit / (1.0f - logit));\n"
"      tmp += logit * gWeight[WeightStartInd + InNodeNum * i + x];\n"
"   }\n"
"   gInNode[x] = tmp;\n"
"}\n";

