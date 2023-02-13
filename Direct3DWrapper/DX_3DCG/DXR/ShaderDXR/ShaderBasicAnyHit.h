///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicAnyHit.hlsl                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderBasicAnyHit =

"[shader(\"anyhit\")]\n"
"void anyBasicHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)\n"
"{\n"
//テクスチャ取得
"    Vertex3 v3 = getVertex();\n"
"    float4 difTex = getDifPixel(attr, v3);\n"
"    float Alpha = difTex.w;\n"

"    if (Alpha <= 0.0f)\n"
"    {\n"
"        IgnoreHit();\n"
"    }\n"
"}\n";
