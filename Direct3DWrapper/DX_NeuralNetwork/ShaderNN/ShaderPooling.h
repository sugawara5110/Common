///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        ShaderPooling.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderPooling =
"RWStructuredBuffer<float> gInput : register(u0);\n"
"RWStructuredBuffer<float> gOutput : register(u1);\n"
"RWStructuredBuffer<float> gInErr : register(u2);\n"
"RWStructuredBuffer<float> gOutErr : register(u3);\n"//Max値の位置格納にも使用, Max値の位置を1.0f,それ以外0.0f

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

//順伝播
//出力側を並列処理(X, Y * Filter, inset), 入力側をループ
"[numthreads(?**, ?**, 1)]\n"//最大X * Y * Z = 1024
"void POFPCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint setInd = outid.z;\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   uint OutsetInd = ((uint)gWidHei.x / POOL) * ((uint)gWidHei.y / POOL) * (uint)gWidHei.z * setInd;\n"
"   uint ox = outid.x;\n"//出力x座標
"   uint oy = outid.y;\n"//出力y座標
"   uint ix = ox * POOL;\n"//pooling対象左上要素x座標
"   uint iy = oy * POOL;\n"//pooling対象左上要素y座標
"   float tmp = gInput[InsetInd + gWidHei.x * iy + ix];\n"//pooling対象4マス左上の値を最初に保持しておく
"   gOutErr[InsetInd + gWidHei.x * iy + ix] = 1.0f;\n"//この値を現時点で一番高い値とする
"   uint errx = 0;\n"
"   uint erry = 0;\n"
"   for(uint i = 1; i < POOL * POOL; i++)\n"//一番最初の要素は↑で取得済みなので1から
"   {\n"
"      uint px = i % POOL;\n"//pool,x座標
"      uint py = i / POOL;\n"//pool,y座標
"      float tmp2 = gInput[InsetInd + gWidHei.x * (iy + py) + (ix + px)];\n"
"      if(tmp < tmp2)\n"
"      {\n"
          //新しい値tmp2が高い場合
"         tmp = tmp2;\n"
"         gOutErr[InsetInd + gWidHei.x * (iy + erry) + (ix + errx)] = 0.0f;\n"//元の値が低かったので0.0f
"         gOutErr[InsetInd + gWidHei.x * (iy + py) + (ix + px)] = 1.0f;\n"//新しい値が高いので新たに1.0fとする
"         errx = px;\n"
"         erry = py;\n"
"      }\n"
"      else\n"
"      {\n"
          //元の値tmpが高い,又は同じ場合
"         gOutErr[InsetInd + gWidHei.x * (iy + py) + (ix + px)] = 0.0f;\n"//新しい値を0.0f
"      }\n"
"   }\n"
"   gOutput[OutsetInd + ((uint)gWidHei.x / POOL) * oy + ox] = tmp;\n"//一番高い値を出力
"}\n"

//逆伝播
//Err入力側を並列処理,Err出力側をループ
"[numthreads(?**, ?**, 1)]\n"//最大X * Y * Z = 1024
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
"      if(gOutErr[InsetInd + gWidHei.x * (oy + py) + (ox + px)] == 1.0f)\n"//一番高い値を出力したニューロンのみ逆伝播行う
"      {\n"
"         gOutErr[InsetInd + gWidHei.x * (oy + py) + (ox + px)] = gInErr[OutsetInd + (gWidHei.x / POOL) * iy + ix];\n"
"      }\n"
"   }\n"
"}\n";

