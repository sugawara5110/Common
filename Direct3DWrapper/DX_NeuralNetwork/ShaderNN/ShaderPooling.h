///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        ShaderPooling.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderPooling =
"RWStructuredBuffer<float> gInput : register(u0);\n"
"RWStructuredBuffer<float> gOutput : register(u1);\n"
"RWStructuredBuffer<float> gInErr : register(u2);\n"
"RWStructuredBuffer<float> gOutErr : register(u3);\n"//Max�l�̈ʒu�i�[�ɂ��g�p, Max�l�̈ʒu��1.0f,����ȊO0.0f

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gWidHei;\n"
"};\n"

"#define POOL 2\n"

//Dispatch(APP��)(X1, Y1, Z1)numthreads(CS��)(X, Y, Z)
//x,y,z,x1,y1,z1 �́`�Ԗ�
//X,Y,Z,X1,Y1,Z1 ��Max�l
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint ���̑�uint3

//���`�d
//�o�͑�����񏈗�(X, Y * Filter, inset), ���͑������[�v
"[numthreads(?**, ?**, 1)]\n"//�ő�X * Y * Z = 1024
"void POFPCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint setInd = outid.z;\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   uint OutsetInd = ((uint)gWidHei.x / POOL) * ((uint)gWidHei.y / POOL) * (uint)gWidHei.z * setInd;\n"
"   uint ox = outid.x;\n"//�o��x���W
"   uint oy = outid.y;\n"//�o��y���W
"   uint ix = ox * POOL;\n"//pooling�Ώۍ���v�fx���W
"   uint iy = oy * POOL;\n"//pooling�Ώۍ���v�fy���W
"   float tmp = gInput[InsetInd + gWidHei.x * iy + ix];\n"//pooling�Ώ�4�}�X����̒l���ŏ��ɕێ����Ă���
"   gOutErr[InsetInd + gWidHei.x * iy + ix] = 1.0f;\n"//���̒l�������_�ň�ԍ����l�Ƃ���
"   uint errx = 0;\n"
"   uint erry = 0;\n"
"   for(uint i = 1; i < POOL * POOL; i++)\n"//��ԍŏ��̗v�f�́��Ŏ擾�ς݂Ȃ̂�1����
"   {\n"
"      uint px = i % POOL;\n"//pool,x���W
"      uint py = i / POOL;\n"//pool,y���W
"      float tmp2 = gInput[InsetInd + gWidHei.x * (iy + py) + (ix + px)];\n"
"      if(tmp < tmp2)\n"
"      {\n"
          //�V�����ltmp2�������ꍇ
"         tmp = tmp2;\n"
"         gOutErr[InsetInd + gWidHei.x * (iy + erry) + (ix + errx)] = 0.0f;\n"//���̒l���Ⴉ�����̂�0.0f
"         gOutErr[InsetInd + gWidHei.x * (iy + py) + (ix + px)] = 1.0f;\n"//�V�����l�������̂ŐV����1.0f�Ƃ���
"         errx = px;\n"
"         erry = py;\n"
"      }\n"
"      else\n"
"      {\n"
          //���̒ltmp������,���͓����ꍇ
"         gOutErr[InsetInd + gWidHei.x * (iy + py) + (ix + px)] = 0.0f;\n"//�V�����l��0.0f
"      }\n"
"   }\n"
"   gOutput[OutsetInd + ((uint)gWidHei.x / POOL) * oy + ox] = tmp;\n"//��ԍ����l���o��
"}\n"

//�t�`�d
//Err���͑�����񏈗�,Err�o�͑������[�v
"[numthreads(?**, ?**, 1)]\n"//�ő�X * Y * Z = 1024
"void POBPCS(uint3 inerrid : SV_DispatchThreadID)\n"
"{\n"
"   uint setInd = inerrid.z;\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   uint OutsetInd = ((uint)gWidHei.x / POOL) * ((uint)gWidHei.y / POOL) * gWidHei.z * setInd;\n"
"   uint ix = inerrid.x;\n"//inErrX
"   uint iy = inerrid.y;\n"//inErrY
"   uint ox = ix * POOL;\n"//outErrX
"   uint oy = iy * POOL;\n"//outErrY
"   for(uint i = 0; i < POOL * POOL; i++)\n"
"   {\n"
"      uint px = i % POOL;\n"
"      uint py = i / POOL;\n"
"      if(gOutErr[InsetInd + gWidHei.x * (oy + py) + (ox + px)] == 1.0f)\n"//��ԍ����l���o�͂����j���[�����̂݋t�`�d�s��
"      {\n"
"         gOutErr[InsetInd + gWidHei.x * (oy + py) + (ox + px)] = gInErr[OutsetInd + (gWidHei.x / POOL) * iy + ix];\n"
"      }\n"
"   }\n"
"}\n";

