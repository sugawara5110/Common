///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderEmissiveMiss.hlsl                                     //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderEmissiveMiss =

"[shader(\"miss\")]\n"
"void emissiveMiss(inout RayPayload payload)\n"
"{\n"
"    payload.color = float3(0.0, 0.0, 0.0);\n"
"    payload.hit = false;\n"
"    payload.reTry = false;\n"
"}\n";
