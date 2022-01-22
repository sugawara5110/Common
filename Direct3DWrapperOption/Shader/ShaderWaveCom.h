///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderWaveCom.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderWaveCom =
"struct WaveData\n"
"{\n"
"	float sinWave;\n"
"   float theta;\n"
"};\n"

"RWStructuredBuffer<WaveData> gInput : register(u0, space0);\n"
"RWStructuredBuffer<WaveData> gOutput : register(u1, space0);\n"

//wave
"cbuffer cbWave  : register(b0, space0)\n"
"{\n"
//x:waveHeight, y:������
"    float4 g_wHei_divide;\n"
"    float g_speed;\n"
"};\n"

//Dispatch(APP��)(X1, Y1, Z1)numthreads(CS��)(X, Y, Z)
//x,y,z,x1,y1,z1 �́`�Ԗ�
//X,Y,Z,X1,Y1,Z1 ��Max�l
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint ���̑�uint3

"[numthreads(4, 4, 1)]\n"
"void CS_Ripples(int3 dtid : SV_DispatchThreadID)\n"
"{\n"
"   int x = dtid.x;"
"   int y = dtid.y;"
"   int div = g_wHei_divide.y;\n"
"   float distance = length(float3(32.0f, 32.0f, 0) - float3(x, y, 0));\n"
"	gOutput[div * y + x].theta = distance % 36;\n"
"	gOutput[div * y + x].sinWave = (sin(gOutput[div * y + x].theta) + 2.0f) * g_wHei_divide.x;\n"
"}\n"

"[numthreads(4, 4, 1)]\n"
"void CS(int3 dtid : SV_DispatchThreadID)\n"
"{\n"
"   int x = dtid.x;"
"   int y = dtid.y;"
"   int div = g_wHei_divide.y;\n"
"	gOutput[div * y + x].theta = (gInput[div * y + x].theta + g_speed) % 360;\n"
"	gOutput[div * y + x].sinWave = (sin(gOutput[div * y + x].theta) + 1.01f) * g_wHei_divide.x;\n"//1.01�̓}�C�i�X�h��
"}\n";
