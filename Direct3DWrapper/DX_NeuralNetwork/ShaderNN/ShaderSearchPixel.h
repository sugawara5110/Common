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

"RWStructuredBuffer<SearchPixelData> gInPixPos : register(u0);\n"//���͉摜�����ʒu
"RWStructuredBuffer<float> gInput : register(u1);\n"//�O���[�X�P�[���ςݓ��͉摜
"RWStructuredBuffer<float> gOutput : register(u2);\n"//InPixCS�ł̕����ς݉摜
"RWStructuredBuffer<float> gOutInd : register(u3);\n"//InPixCS�ł̕�����s�N�Z���z�u�ʒu
"RWStructuredBuffer<float> gNNoutput : register(u4);\n"//NN�����output, �������Ɠ���
"RWTexture2D<float4> gInputCol : register(u5); \n"//����3ch�摜, OutPixCS�ł̌��o�ӏ��g�ǉ��ς݉摜

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gInpixWH_OutpixWH;\n"
"    float4 gseapixWH_step_PDNum;\n"
"    float4 gThreshold;\n"
"};\n"

//srcPixel����, �X���b�h��:out���s�N�Z�����Ɠ���
"[numthreads(1, 1, 1)]\n"//�ő�X * Y * Z = 1024
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

//srcPixel�֌��o�ӏ��g�ǉ�, �X���b�h��:srcPixel����
"[numthreads(1, 1, 1)]\n"//�ő�X * Y * Z = 1024
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