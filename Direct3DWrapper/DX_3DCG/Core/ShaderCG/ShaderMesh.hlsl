///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderMESH.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommonParameters.hlsl"

//****************************************���b�V�����_**************************************************************//
GS_Mesh_INPUT VSMesh(float3 Pos : POSITION, float3 Nor : NORMAL, float3 Tan : TANGENT, 
                     float2 Tex : TEXCOORD, uint InstanceID : SV_InstanceID)
{
    GS_Mesh_INPUT output = (GS_Mesh_INPUT) 0;

    output.Pos = float4(Pos, 1);
    output.Nor = Nor;
    output.Tan = Tan;
    
    //instanceID�؂�ւ�, DXR �e�Z���[�V�������̂�
    uint instanceID = InstanceID;
    if (g_instanceID.y == 1.0f)
    {
        instanceID = g_instanceID.x;
    }
        
    float4 pXpYmXmY = wvpCb[instanceID].pXpYmXmY;
    output.Tex0.x = Tex.x * pXpYmXmY.x + pXpYmXmY.x * pXpYmXmY.z;
    output.Tex0.y = Tex.y * pXpYmXmY.y + pXpYmXmY.y * pXpYmXmY.w;
    output.Tex1 = output.Tex0;
    output.instanceID = instanceID;

    return output;
}
//****************************************���b�V�����_**************************************************************//

