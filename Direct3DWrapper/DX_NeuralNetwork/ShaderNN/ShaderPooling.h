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

//Dispatch(APP‘¤)(X1, Y1, Z1)numthreads(CS‘¤)(X, Y, Z)
//x,y,z,x1,y1,z1 ‚Í`”Ô–Ú
//X,Y,Z,X1,Y1,Z1 ‚ÍMax’l
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint ‚»‚Ì‘¼uint3

//‡“`”d
//o—Í‘¤‚ğ•À—ñˆ—,“ü—Í‘¤‚ğƒ‹[ƒv
"[numthreads(1, 1, 1)]\n"//Å‘åX * Y * Z = 1024
"void POFPCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint setInd = outid.z;\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   uint OutsetInd = (gWidHei.x / POOL) * (gWidHei.y / POOL) * gWidHei.z * setInd;\n"
"   uint ox = outid.x;\n"
"   uint oy = outid.y;\n"
"   uint ix = ox * POOL;\n"
"   uint iy = oy * POOL;\n"
"   float tmp = gInput[InsetInd + gWidHei.x * iy + ix];\n"
"   uint errx = 0;\n"
"   uint erry = 0;\n"
"   for(uint i = 1; i < POOL * POOL; i++)\n"
"   {\n"
"      uint px = i % POOL;\n"
"      uint py = i / POOL;\n"
"      float tmp2 = gInput[InsetInd + gWidHei.x * (iy + py) + (ix + px)];\n"
"      if(tmp < tmp2)\n"
"      {\n"
"         tmp = tmp2;\n"
"         gOutErr[InsetInd + gWidHei.x * (iy + erry) + (ix + errx)] = 0.0f;\n"
"         gOutErr[InsetInd + gWidHei.x * (iy + py) + (ix + px)] = 1.0f;\n"
"         errx = px;\n"
"         erry = py;\n"
"      }\n"
"      else\n"
"      {\n"
"         gOutErr[InsetInd + gWidHei.x * (iy + py) + (ix + px)] = 0.0f;\n"
"      }\n"
"   }\n"
"   gOutput[OutsetInd + (gWidHei.x / POOL) * oy + ox] = tmp;\n"
"}\n"

//‹t“`”d
//Err“ü—Í‘¤‚ğ•À—ñˆ—,Erro—Í‘¤‚ğƒ‹[ƒv
"[numthreads(1, 1, 1)]\n"//Å‘åX * Y * Z = 1024
"void POBPCS(uint3 inerrid : SV_DispatchThreadID)\n"
"{\n"
"   uint setInd = inerrid.z;\n"
"   uint InsetInd = gWidHei.x * gWidHei.y * gWidHei.z * setInd;\n"
"   uint OutsetInd = (gWidHei.x / POOL) * (gWidHei.y / POOL) * gWidHei.z * setInd;\n"
"   uint ix = inerrid.x;\n"
"   uint iy = inerrid.y;\n"
"   uint ox = ix * POOL;\n"
"   uint oy = iy * POOL;\n"
"   for(uint i = 0; i < POOL * POOL; i++)\n"
"   {\n"
"      uint px = i % POOL;\n"
"      uint py = i / POOL;\n"
"      if(gOutErr[InsetInd + gWidHei.x * (oy + py) + (ox + px)] == 1.0f)\n"
"      {\n"
"         gOutErr[InsetInd + gWidHei.x * (oy + py) + (ox + px)] = gInErr[OutsetInd + (gWidHei.x / POOL) * iy + ix];\n"
"      }\n"
"   }\n"
"}\n";

