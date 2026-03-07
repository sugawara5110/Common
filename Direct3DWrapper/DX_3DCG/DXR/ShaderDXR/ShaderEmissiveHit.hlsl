///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderEmissiveHit.hlsl                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommon.hlsl"

int getEmissiveIndex()
{
    int ret = -1;
    for (int i = 0; i < LIGHT_PCS; i++)
    {
        if (emissiveNo[i].x == InstanceID())
        {
            ret = i;
            break;
        }
    }
    return ret;
}

[shader("closesthit")]
void EmissiveHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    Vertex3 v3 = getVertex();
    float4 difTex = getDifPixel(attr, v3);
    float3 normalMap = getNorPixel(attr, v3);
    payload.normal = normalMap;
    payload.hitPosition = HitWorldPosition();

    if (difTex.w < 1.0f)
    {
        payload.reTry = true;
        return;
    }

    payload.hitInstanceId = (int) getInstancingID();
    payload.EmissiveIndex = getEmissiveIndex();
    payload.hit = true;
    payload.mNo = getMaterialCB().materialNo;
    
    const uint materialID = getMaterialID();
    const MaterialCB mcb = material[materialID];
    payload.color = mcb.Diffuse.xyz * difTex.xyz;
}
