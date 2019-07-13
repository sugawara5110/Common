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
"RWStructuredBuffer<float> gGradient : register(u7);\n"//勾配
"RWStructuredBuffer<float> gGradBias : register(u8);\n"//bias勾配

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gWidHei;\n"//入力w:x, 入力h:y ,FilNum:z
"    float4 gfilWid_filStep;\n"//Filwid数:x, Filstep数:y
"    float4 gLear_inputS;\n"//学習率:x, inputset数:y, bias学習率:z
"};\n"

//Dispatch(APP側)(X1, Y1, Z1)numthreads(CS側)(X, Y, Z)
//x,y,z,x1,y1,z1 は～番目
//X,Y,Z,X1,Y1,Z1 はMax値
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint その他uint3

//順伝播
//出力側を並列処理,入力側をループ(スレッド数は出力側と同数)
"[numthreads(?**, ?**, 1)]\n"//最大X * Y * Z = 1024
"void CNFPCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   int outwid = gWidHei.x / gfilWid_filStep.y;\n"//出力wid数
"   int outhei = gWidHei.y / gfilWid_filStep.y;\n"//出力hei数
"   uint setInd = outid.z;\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   uint OutsetInd = outwid * outhei * gWidHei.z * setInd;\n"
"   int ox = outid.x;\n"//出力widIndex
"   int oy = outid.y % outhei;\n"//出力heiIndex(fil除き)
"   int ix = ox * gfilWid_filStep.y;\n"//入力widIndex
"   int iy = oy * gfilWid_filStep.y;\n"//入力heiIndex
"   int padding = gfilWid_filStep.x / 2;\n"//フィルターwid/2の値をpadding数とする
"   uint filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"//Filter要素数
"   uint numInd = outid.y / outhei;\n"//スレッドY側のFilter個数単位のindex
"   uint filStInd = numInd * filElNum;\n"//Filter配列のFilter個数単位のindex * 要素数
"   uint inStInd = numInd * gWidHei.x * gWidHei.y;\n"//Input配列のFilter個数単位のindex * 要素数
"   uint outStInd = numInd * outwid * outhei;\n"//Output配列のFilter個数単位のindex * 要素数
"   uint FilNumInd = filStInd / filElNum;\n"//現Filterのindex(要素indexではない)

"   float tmp = 0.0f;\n"
"   for(uint i = 0; i < filElNum; i++)\n"
"   {\n"
"      int fx = (i % gfilWid_filStep.x) - padding;\n"//Filter座標X - padding
"      int fy = (i / gfilWid_filStep.x) - padding;\n"//Filter座標Y - padding
"      if(iy + fy >= 0 && iy + fy < gWidHei.y && ix + fx >= 0 && ix + fx < gWidHei.x)\n"//Padding領域はスキップ
"      {\n"
"         tmp += gInput[InsetInd + inStInd + gWidHei.x * (iy + fy) + (ix + fx)] * gFilter[filStInd + i] * \n"
"                gDropOutF[inStInd + gWidHei.x * (iy + fy) + (ix + fx)] + gBias[FilNumInd];\n"
"      }\n"
"   }\n"
"   gOutput[OutsetInd + outStInd + outwid * oy + ox] = tmp;\n"
"}\n"

//逆伝播
//Err出力側を並列処理,Err入力側をループ(スレッド数は入力数と同数)
"[numthreads(?**, ?**, 1)]\n"//最大X * Y * Z = 1024   
"void CNBPCS1(uint3 inid : SV_DispatchThreadID)\n"
"{\n"
"   int inwid = gWidHei.x / gfilWid_filStep.y;\n"//output,inerr側wid数
"   int inhei = gWidHei.y / gfilWid_filStep.y;\n"//output,inerr側hei数
"   int ix = inid.x / gfilWid_filStep.y;\n"//output,inerr, widIndex
"   int iy = (inid.y  / gfilWid_filStep.y) % inhei;\n"//output,inerr, heiIndex(filter含まない)
"   int ox = ix * gfilWid_filStep.y;\n"//input, outerr, widIndex
"   int oy = iy * gfilWid_filStep.y;\n"//input, outerr, heiIndex(filter含まない)
"   int padding = gfilWid_filStep.x / 2;\n"
"   int filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"//filter要素数
"   int numInd = inid.y / gWidHei.y;\n"
"   int filStInd = numInd * filElNum;\n"
"   int inStInd = numInd * inwid * inhei;\n"//inerr, Filter個数単位のindex * 要素数
"   int outStInd = numInd * gWidHei.x * gWidHei.y;\n"//outerr, Filter個数単位のindex * 要素数
"   uint setInd = inid.z;\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   uint OutsetInd = inwid * inhei * gWidHei.z * setInd;\n"

"   float tmp = 0.0f;\n"
"   for(int i = filElNum - 1; i >= 0; i--)\n"
"   {\n"
"      int fx = (i % gfilWid_filStep.x) - padding;\n"
"      int fy = (i / gfilWid_filStep.x) - padding;\n"
"      if(iy + fy >= 0 && iy + fy < inhei && ix + fx >= 0 && ix + fx < inwid)\n"//Padding領域はスキップ
"      {\n"
"         uint inErrInd = OutsetInd + inStInd + inwid * (iy + fy) + (ix + fx);\n"
"         tmp += gInErr[inErrInd] * gFilter[filStInd + i];\n"
"      }\n"
"   }\n"
"   gOutErr[InsetInd + outStInd + gWidHei.x * oy + ox] = tmp * gDropOutF[outStInd + gWidHei.x * oy + ox];\n"
"}\n"

"uint CNBPCS2sub(int2 inid)\n"
"{\n"
"   int filx = inid.x;\n"//Filter要素XIndex
"   int fily = inid.y % gfilWid_filStep.x;\n"//Filter要素YIndex(Filter個数含まない)
"   int padding = gfilWid_filStep.x / 2;\n"
"   uint inErrwid = gWidHei.x / gfilWid_filStep.y;\n"
"   uint inErrhei = gWidHei.y / gfilWid_filStep.y;\n"
"   uint numInd = inid.y / gfilWid_filStep.x;\n"
"   uint filElNum = gfilWid_filStep.x * gfilWid_filStep.x;\n"
"   uint filStInd = numInd * filElNum;\n"
"   uint inEStInd = numInd * inErrwid * inErrhei;\n"
"   uint inStInd = numInd * gWidHei.x * gWidHei.y;\n"

"   float tmpSum = 0.0f;\n"
"   for(int k = 0; k < gLear_inputS.y; k++)\n"//inputSet数でloop
"   {\n"
"      uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * k;\n"
"      uint OutsetInd = inErrwid * inErrhei * gWidHei.z * k;\n"
"      float tmp = 0.0f;\n"
"      for(uint i = 0; i < inErrwid * inErrhei; i+=gfilWid_filStep.y)\n"
"      {\n"
"         int Ex = i % inErrwid;\n"//inerrX
"         int Ey = i / inErrwid;\n"//inerrY
"         int Ix = Ex * gfilWid_filStep.y + (filx - padding);\n"//inputX
"         int Iy = Ey * gfilWid_filStep.y + (fily - padding);\n"//inputY

"         if(Ix >= 0 && Ix < gWidHei.x && Iy >= 0 && Iy < gWidHei.y)\n"
"         {\n"
"            float inE = gInErr[OutsetInd + inEStInd + inErrwid * Ey + Ex];\n"
"            tmp += (inE * gInput[InsetInd + inStInd + gWidHei.x * Iy + Ix]);\n"
"         }\n"
"      }\n"
"      tmpSum += tmp;\n"
"   }\n"
"   float tmpAve = tmpSum / gLear_inputS.y;\n"
"   uint grInd = filStInd + gfilWid_filStep.x * fily + filx;\n"
"   gGradient[grInd] = tmpAve;\n"
"   return grInd;\n"
"}\n"

"#define BP_X ?**\n"
"#define BP_Y ?**\n"
//フィルタ更新
//フィルタを並列処理(スレッド数はフィルター要素数 * フィルター数と同数)
"[numthreads(BP_X, BP_Y, 1)]\n"//最大X * Y * Z = 1024   
"void CNBPCS2(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   uint ind = CNBPCS2sub(inid);\n"
"   gFilter[ind] -= gGradient[ind] * gLear_inputS.x;\n"
"}\n"
//フィルタ更新無し
"[numthreads(BP_X, BP_Y, 1)]\n"//最大X * Y * Z = 1024   
"void CNBPCS2NoFilterUpdate(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   CNBPCS2sub(inid);\n"
"}\n"

//bias更新
//Filter毎に並列処理
"[numthreads(?**, 1, 1)]\n"//最大X * Y * Z = 1024  
"void CNBPCSBias(int2 filid : SV_DispatchThreadID)\n"
"{\n"
"   int inwid = gWidHei.x / gfilWid_filStep.y;\n"//下層からの誤差wid
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
"   gGradBias[filid.x] = tmp / gLear_inputS.y;\n"
"   gBias[filid.x] -= gGradBias[filid.x] * gLear_inputS.z;\n"
"}\n";


