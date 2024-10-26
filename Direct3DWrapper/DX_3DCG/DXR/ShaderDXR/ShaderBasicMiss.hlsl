
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBasicMiss.hlsl                                        //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommon.hlsl"

[shader("miss")]
void basicMiss(inout RayPayload payload)
{
    payload.color = float3(0.0, 0.0, 0.0);
    payload.hit = false;
    payload.reTry = false;

    if (useImageBasedLighting)
    {
        payload.color = getSkyLight(mul(WorldRayDirection(), (float3x3) ImageBasedLighting_Matrix));
        payload.hitPosition = HitWorldPosition();
        payload.hit = true;
    }
}