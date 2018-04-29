///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                    ShaderConvolution.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderConvolution =
"RWStructuredBuffer<float> gInput : register(u0);\n"
"RWStructuredBuffer<float> gOutput : register(u1);\n"
"RWStructuredBuffer<float> gInErr : register(u2);\n"
"RWStructuredBuffer<float> gOutErr : register(u3);\n"
"RWStructuredBuffer<float> gFilter : register(u4);\n"

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gWidHei;\n"
"    float4 gfilWid_filStep;\n"
"    float4 gLear;\n"//学習率:x
"};\n"

//Dispatch(APP側)(X1, Y1, Z1)numthreads(CS側)(X, Y, Z)
//x,y,z,x1,y1,z1 は～番目
//X,Y,Z,X1,Y1,Z1 はMax値
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint その他uint3

//順伝搬
//出力側を並列処理,入力側をループ(スレッド数は出力側と同数)
"[numthreads(1, 1, 1)]\n"//最大X * Y * Z = 1024
"void CNFPCS(int3 outid : SV_DispatchThreadID)\n"
"{\n"
"   int outwid = gWidHei.x / gfilWid_filStep.y;\n"
"   int outhei = gWidHei.y / gfilWid_filStep.y;\n"
"   uint detecInd = outid.z;\n"
"   uint InDetecInd = gWidHei.x * gWidHei.y * gWidHei.z * detecInd;\n"
"   uint OutDetecInd = outwid * outhei * gWidHei.z * detecInd;\n"
"   int ox = outid.x;\n"
"   int oy = outid.y % outhei;\n"
"   int ix = ox * gfilWid_filStep.y;\n"
"   int iy = oy * gfilWid_filStep.y;\n"
"   int padding = gfilWid_filStep.x / 2;\n"
"   int filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"
"   int numInd = outid.y * gfilWid_filStep.y / gWidHei.y;\n"
"   int filStInd = numInd * filElNum;\n"
"   int inStInd = numInd * gWidHei.x * gWidHei.y;\n"
"   int outStInd = numInd * outwid * outhei;\n"

"   float tmp = 0.0f;\n"
"   for(int i = 0; i < filElNum; i++)\n"
"   {\n"
"      int fx = (i % gfilWid_filStep.x) - padding;\n"
"      int fy = (i / gfilWid_filStep.x) - padding;\n"
"      if(iy + fy >= 0 && iy + fy < gWidHei.y && ix + fx >= 0 && ix + fx < gWidHei.x)\n"//Padding領域はスキップ
"      {\n"
"         tmp += gInput[InDetecInd + inStInd + gWidHei.x * (iy + fy) + (ix + fx)] * gFilter[filStInd + i];\n"
"      }\n"
"   }\n"
"   float sig = 1.0f / (1.0f + pow(2.71828182846, -tmp));\n"
"   gOutput[OutDetecInd + outStInd + outwid * oy + ox] = sig;\n"
"}\n"

//gOutErr初期化
"[numthreads(1, 1, 1)]\n"//最大X * Y * Z = 1024
"void CNBPCS0(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   int x = inid.x;\n"
"   int y = inid.y;\n"
"   gOutErr[gWidHei.x * y + x] = 0.0f;\n"
"}\n"

//逆伝搬
//Err出力側を並列処理,Err入力側をループ(スレッド数は入力数と同数)
"[numthreads(1, 1, 1)]\n"//最大X * Y * Z = 1024
"void CNBPCS1(int2 inid : SV_DispatchThreadID)\n"
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

"   float tmp = 0.0f;\n"
"   for(int i = filElNum - 1; i >= 0; i--)\n"
"   {\n"
"      int fx = (i % gfilWid_filStep.x) - padding;\n"
"      int fy = (i / gfilWid_filStep.x) - padding;\n"
"      if(iy + fy >= 0 && iy + fy < inhei && ix + fx >= 0 && ix + fx < inwid)\n"//Padding領域はスキップ
"      {\n"
"         tmp += gInErr[inStInd + inwid * (iy + fy) + (ix + fx)] * gFilter[filStInd + i];\n"
"      }\n"
"   }\n"
"   gOutErr[outStInd + gWidHei.x * oy + ox] = tmp;\n"
"}\n"

//フィルタ更新
//フィルタを並列処理(スレッド数はフィルター要素数 * フィルター数と同数)
"[numthreads(1, 1, 1)]\n"//最大X * Y * Z = 1024
"void CNBPCS2(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   int filx = inid.x;\n"
"   int fily = inid.y % gfilWid_filStep.x;\n"
"   int padding = gfilWid_filStep.x / 2;\n"
"   int inErrwid = gWidHei.x / gfilWid_filStep.y;\n"
"   int inErrhei = gWidHei.y / gfilWid_filStep.y;\n"
"   int numInd = inid.y / gfilWid_filStep.x;\n"
"   int filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"
"   int filStInd = numInd * filElNum;\n"
"   int inEStInd = numInd * inErrwid * inErrhei;\n"
"   int inStInd = numInd * gWidHei.x * gWidHei.y;\n"

"   float tmp = 0.0f;\n"
"   for(int i = 0; i < inErrwid * inErrhei; i+=gfilWid_filStep.y)\n"
"   {\n"
"      int Ex = i % inErrwid;\n"
"      int Ey = i / inErrwid;\n"
"      int Ix = Ex * gfilWid_filStep.y + (filx - padding);\n"
"      int Iy = Ey * gfilWid_filStep.y + (fily - padding);\n"

"      if(Ix >= 0 && Ix < gWidHei.x && Iy >= 0 && Iy < gWidHei.y)\n"
"      {\n"
"         tmp += gInErr[inEStInd + inErrwid * Ey + Ex] * \n"
"                gInput[inStInd + gWidHei.x * Iy + Ix];\n"
"      }\n"
"   }\n"
"   gFilter[filStInd + gfilWid_filStep.x * fily + filx] += tmp * gLear.x;\n"
"}\n";


