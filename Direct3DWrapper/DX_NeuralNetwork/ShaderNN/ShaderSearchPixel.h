///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        ShaderSearchPixel.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderSearchPixel =
"struct SearchPixelData\n"
"{\n"
"	uint stW;\n"
"	uint stH;\n"
"	uint enW;\n"
"	uint enH;\n"
"};\n"

"RWStructuredBuffer<SearchPixelData> gInPixPos : register(u0);\n"//入力画像分割位置
"RWStructuredBuffer<float> gInput : register(u1);\n"//グレースケール済み入力画像
"RWStructuredBuffer<float> gOutput : register(u2);\n"//InPixCSでの分割済み画像
"RWStructuredBuffer<float> gOutInd : register(u3);\n"//InPixCSでの分割後ピクセル配置位置
"RWStructuredBuffer<float> gNNoutput : register(u4);\n"//NNからのoutput, 分割数と同数
"RWTexture2D<float4> gInputCol : register(u5); \n"//入力3ch画像, OutPixCSでの検出箇所枠追加済み画像

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gInpixWH_OutpixWH;\n"
"    float4 gseapixWH_step_PDNum;\n"
"    float4 gThreshold;\n"
"};\n"

//srcPixel入力, スレッド数:out側ピクセル数と同数
"[numthreads(1, 1, 1)]\n"//最大X * Y * Z = 1024
"void InPixCS(uint2 inid : SV_DispatchThreadID)\n"
"{\n"
"   uint outpx = inid.x;\n"
"   uint outpy = inid.y;\n"
"   uint outbwidNum = gInpixWH_OutpixWH.z / gseapixWH_step_PDNum.x;\n"
"   uint outInd = gInpixWH_OutpixWH.z * outpy + outpx;\n"
"   uint outbxInd = outpx / gseapixWH_step_PDNum.x;\n"
"   uint outbyInd = outpy / gseapixWH_step_PDNum.y;\n"
"   uint blockInd = outbwidNum * outbyInd + outbxInd;\n"
"   uint bx = outpx % gseapixWH_step_PDNum.x;\n"
"   uint by = outpy % gseapixWH_step_PDNum.y;\n"
"   uint inx = gInPixPos[blockInd].stW + bx;\n"
"   uint iny = gInPixPos[blockInd].stH + by;\n"
"   gOutput[gOutInd[outInd]] = gInput[gInpixWH_OutpixWH.x * iny + inx];\n"
"}\n"

//srcPixelへ検出箇所枠追加, スレッド数:srcPixel同数
"[numthreads(1, 1, 1)]\n"//最大X * Y * Z = 1024
"void OutPixCS(uint2 inid : SV_DispatchThreadID)\n"
"{\n"
"   uint outpx = inid.x;\n"
"   uint outpy = inid.y;\n"

"   for(uint i = 0; i < gseapixWH_step_PDNum.w; i++)\n"
"   {\n"
"      uint stx = gInPixPos[i].stW;\n"
"      uint sty = gInPixPos[i].stH;\n"
"      uint enx = gInPixPos[i].enW;\n"
"      uint eny = gInPixPos[i].enH;\n"
"      if(gNNoutput[i] > gThreshold.x && \n"
"           (\n"
"             outpy >= sty && outpy <= eny && ((outpx >= stx && outpx <= stx + 3) || (outpx <= enx && outpx >= enx - 3)) ||\n"
"             outpx >= stx && outpx <= enx && ((outpy >= sty && outpy <= sty + 3) || (outpy <= eny && outpy >= eny - 3))"
"           )\n"
"      )\n"
"      {\n"
"         float4 col = float4(0.0f, 0.0f, 1.0f, 1.0f);\n"
"         gInputCol[inid] = col;\n"
"      }\n"
"   }\n"
"}\n";