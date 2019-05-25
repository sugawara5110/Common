///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                    ShaderGradCAM.hlsl                                                 //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderGradCAM =
"RWStructuredBuffer<float> gInputFeatureMapBuffer : register(u0);\n"
"RWStructuredBuffer<float> gInputGradientBuffer : register(u1);\n"
"RWStructuredBuffer<float> gGlobalAveragePoolingBuffer : register(u2);\n"
"RWStructuredBuffer<float> gOutGradCAMBuffer : register(u3);\n"
"RWStructuredBuffer<float> gOutGradCAMSynthesisBuffer : register(u4);\n"
"RWTexture2D<float4> gInputCol : register(u5); \n"//����3ch�摜,GradCAM����

"cbuffer global  : register(b0)\n"
"{\n"
"    uint NumFil;\n"//�t�B���^�[��
"    uint SizeFeatureMapW;\n"//�����ʃT�C�YW
"    uint SizeFeatureMapH;\n"//�����ʃT�C�YH
"    uint NumConvFilElement;\n"//�t�B���^�[�v�f��
"    uint NumFeatureMapW;\n"//������Map��W
"    uint NumFeatureMapH;\n"//������Map��H
"    uint srcWid;\n"//���o�Ώۉ摜sizeW
"    uint srcHei;\n"//���o�Ώۉ摜sizeH
"    uint srcConvMapW;\n"//1Map���o�͈�W
"    uint srcConvMapH;\n"//1Map���o�͈�H
"    uint MapSlide;\n"//������Map�X���C�h��
"    float SignalStrength;\n"//�M������
"};\n"

//GAP
//�X���b�h���̓t�B���^�[���Ɠ���
"[numthreads(1, 1, 1)]\n"
"void ComGAP(uint3 filid : SV_DispatchThreadID)\n"
"{\n"
"   uint filInd = filid.x;\n"
"   float tmp = 0.0f;\n"
"   for(uint i = 0; i < NumConvFilElement; i++)\n"
"   {\n"
"      tmp += gInputGradientBuffer[NumFil * filInd + i];\n"
"   }\n"
"   gGlobalAveragePoolingBuffer[filInd] = tmp / (float)NumConvFilElement;\n"
"}\n"

//GradCAM
//�X���b�h����FeatureMap�v�f�� �~ inputsetnum
"[numthreads(1, 1, 1)]\n"
"void ComGradCAM(uint3 mapid : SV_DispatchThreadID)\n"
"{\n"
"   uint mapInd = mapid.x;\n"
"   uint inputsetnumInd = mapid.z;\n"
"   float tmp = 0.0f;\n"
"   uint NumConvOutPutEl = SizeFeatureMapW * SizeFeatureMapH;\n"
"   for(uint i = 0; i < NumFil; i++)\n"
"   {\n"
"      uint fmInd = NumConvOutPutEl * NumFil * inputsetnumInd + NumConvOutPutEl * i + mapInd;\n"
"      tmp += gInputFeatureMapBuffer[fmInd] * gGlobalAveragePoolingBuffer[i];\n"
"   }\n"
"   gOutGradCAMBuffer[NumConvOutPutEl * inputsetnumInd + mapInd] = max(0, tmp);\n"
"}\n"

//gOutGradCAMSynthesisBuffer������
//�X���b�h��srcWid * srcHei
"[numthreads(1, 1, 1)]\n"
"void InitGradCAMSynthesis(uint3 pixid : SV_DispatchThreadID)\n"
"{\n"
"   uint ind = srcWid * pixid.y + pixid.x;\n"
"   gOutGradCAMSynthesisBuffer[ind] = 0.0f;\n"
"}\n"

//GradCAM����
//�X���b�h����srcConvMapW * srcConvMapH(1Map���o�͈�)
"[numthreads(1, 1, 1)]\n"
"void ComGradCAMSynthesis(uint3 pixid : SV_DispatchThreadID)\n"
"{\n"
"   uint srcMapx = pixid.x;\n"
"   uint srcMapy = pixid.y;\n"
"   float Xmag = (float)SizeFeatureMapW / (float)srcConvMapW;\n"
"   float Ymag = (float)SizeFeatureMapH / (float)srcConvMapH;\n"
"   uint fMapx = srcMapx * Xmag;\n"
"   uint fMapy = srcMapy * Ymag;\n"
"   for(uint y = 0; y < NumFeatureMapH; y++)\n"
"   {\n"
"      uint mapStPosY = MapSlide * y;\n"
"      for(uint x = 0; x < NumFeatureMapW; x++)\n"
"      {\n"
"         uint mapStPosX = MapSlide * x;\n"
"         uint outInd = srcWid * (mapStPosY + srcMapy) + mapStPosX + srcMapx;\n"
"         uint NumConvOutPutEl = SizeFeatureMapW * SizeFeatureMapH;\n"
"         uint inputsetnumInd = NumFeatureMapW * y + x;\n"
"         uint inInd = NumConvOutPutEl * inputsetnumInd + SizeFeatureMapW * fMapy + fMapx;\n"
"         if(gOutGradCAMSynthesisBuffer[outInd] < gOutGradCAMBuffer[inInd])\n"
"         {\n"
"            gOutGradCAMSynthesisBuffer[outInd] = gOutGradCAMBuffer[inInd];\n"
"         }\n"
"      }\n"
"   }\n"
"}\n"

//�摜����
//�X���b�h����srcWid * srcHei
"[numthreads(1, 1, 1)]\n"
"void ComPixelSynthesis(uint2 pixid : SV_DispatchThreadID)\n"
"{\n"
"   uint ind = srcWid * pixid.y + pixid.x;\n"
"   float4 col = gInputCol[pixid];\n"
"   float red = gOutGradCAMSynthesisBuffer[ind] * SignalStrength;\n"
"   float bul = max(0, 1.0f - red);\n"
"   col.x = (col.x + red) * 0.5f;\n"
"   col.z = (col.z + bul) * 0.5f;\n"
"   gInputCol[pixid] = col;\n"
"}\n";