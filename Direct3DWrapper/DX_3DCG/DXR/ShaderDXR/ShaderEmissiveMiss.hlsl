///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderEmissiveMiss.hlsl                                     //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommon.hlsl"

[shader("miss")]
void EmissiveMiss(inout RayPayload payload)
{
    payload.color = float3(0.0, 0.0, 0.0);
    payload.hit = false;
    payload.reTry = false;
    payload.mNo = NONE;
    payload.DiffuseAlbedo = payload.color;
    payload.roughness = 0.0f;
}
