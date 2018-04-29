///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        ShaderPooling.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderPooling =
"RWStructuredBuffer<float> gInput : register(u0);\n"
"RWStructuredBuffer<float> gOutput : register(u1);\n"
"RWStructuredBuffer<float> gInErr : register(u2);\n"
"RWStructuredBuffer<float> gOutErr : register(u3);\n"

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gWidHei;\n"
"};\n"

"#define POOL 2\n"

//Dispatch(APP側)(X1, Y1, Z1)numthreads(CS側)(X, Y, Z)
//x,y,z,x1,y1,z1 は～番目
//X,Y,Z,X1,Y1,Z1 はMax値
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint その他uint3

//順伝搬
//出力側を並列処理,入力側をループ
"[numthreads(1, 1, 1)]\n"//最大X * Y * Z = 1024
"void POFPCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint detecInd = outid.z;\n"
"   uint InDetecInd = gWidHei.x * gWidHei.y * gWidHei.z * detecInd;\n"
"   uint OutDetecInd = (gWidHei.x / POOL) * (gWidHei.y / POOL) * gWidHei.z * detecInd;\n"
"   uint ox = outid.x;\n"
"   uint oy = outid.y;\n"
"   uint ix = ox * POOL;\n"
"   uint iy = oy * POOL;\n"
"   float tmp = gInput[InDetecInd + gWidHei.x * iy + ix];\n"
"   uint errx = 0;\n"
"   uint erry = 0;\n"
"   for(uint i = 1; i < POOL * POOL; i++)\n"
"   {\n"
"      uint px = i % POOL;\n"
"      uint py = i / POOL;\n"
"      float tmp2 = gInput[InDetecInd + gWidHei.x * (iy + py) + (ix + px)];\n"
"      if(tmp < tmp2)\n"
"      {\n"
"         tmp = tmp2;\n"
"         gOutErr[gWidHei.x * (iy + erry) + (ix + errx)] = 0.0f;\n"
"         gOutErr[gWidHei.x * (iy + py) + (ix + px)] = 1.0f;\n"
"         errx = px;\n"
"         erry = py;\n"
"      }\n"
"      else\n"
"      {\n"
"         gOutErr[gWidHei.x * (iy + py) + (ix + px)] = 0.0f;\n"
"      }\n"
"   }\n"
"   gOutput[OutDetecInd + (gWidHei.x / POOL) * oy + ox] = tmp;\n"
"}\n"

//逆伝搬
//Err入力側を並列処理,Err出力側をループ
"[numthreads(1, 1, 1)]\n"//最大X * Y * Z = 1024
"void POBPCS(uint2 inerrid : SV_DispatchThreadID)\n"
"{\n"
"   uint ix = inerrid.x;\n"
"   uint iy = inerrid.y;\n"
"   uint ox = ix * POOL;\n"
"   uint oy = iy * POOL;\n"
"   for(uint i = 0; i < POOL * POOL; i++)\n"
"   {\n"
"      uint px = i % POOL;\n"
"      uint py = i / POOL;\n"
"      if(gOutErr[gWidHei.x * (oy + py) + (ox + px)] == 1.0f)\n"
"      {\n"
"         gOutErr[gWidHei.x * (oy + py) + (ox + px)] = gInErr[(gWidHei.x / POOL) * iy + ix];\n"
"      }\n"
"   }\n"
"}\n";

