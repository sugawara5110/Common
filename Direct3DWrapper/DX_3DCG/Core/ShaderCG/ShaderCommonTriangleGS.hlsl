///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 ShaderCommonTriangleGS.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ShaderCommonParameters.hlsl"
#include "ShaderNormalTangent.hlsl"

PS_INPUT PsInput(GS_Mesh_INPUT input)
{
	PS_INPUT output = (PS_INPUT) 0;

	output.wPos = mul(input.Pos, wvpCb[input.instanceID].world);
	output.Pos = mul(input.Pos, wvpCb[input.instanceID].wvp);
	output.Tex0 = input.Tex0;
	output.Tex1 = input.Tex1;
	output.instanceID = input.instanceID;
	return output;
}

void Stream(GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream, NormalTangent tan[3])
{
	PS_INPUT p = (PS_INPUT) 0;

	p = PsInput(Input[0]);
	p.Nor = tan[0].normal;
	p.tangent = tan[0].tangent;
	triStream.Append(p);

	p = PsInput(Input[1]);
	p.Nor = tan[1].normal;
	p.tangent = tan[1].tangent;
	triStream.Append(p);

	p = PsInput(Input[2]);
	p.Nor = tan[2].normal;
	p.tangent = tan[2].tangent;
	triStream.Append(p);

	triStream.RestartStrip();
}
//********************************�W�I���g���V�F�[�_�[***************************************************************//
//****************************�O��DS,�m�[�}���}�b�v�L**********************************************//
//********Smooth*********//
[maxvertexcount(3)]
void GS_Before_ds_Smooth(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)
{
	NormalTangent tan[3];
	GS_Mesh_INPUT recal[3];

//DS��@���Čv�Z
	NormalRecalculationEdge(Input);
	recal[0] = NormalRecalculationSmooth(Input[0]);
	recal[1] = NormalRecalculationSmooth(Input[1]);
	recal[2] = NormalRecalculationSmooth(Input[2]);

//�ڃx�N�g���v�Z
	tan[0] = GetTangent(recal[0].Nor, (float3x3) wvpCb[recal[0].instanceID].world, recal[0].Tan);
	tan[1] = GetTangent(recal[1].Nor, (float3x3) wvpCb[recal[1].instanceID].world, recal[1].Tan);
	tan[2] = GetTangent(recal[2].Nor, (float3x3) wvpCb[recal[2].instanceID].world, recal[2].Tan);

	Stream(recal, triStream, tan);
}
//********Smooth*********//
//********Edge***********//
[maxvertexcount(3)]
void GS_Before_ds_Edge(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)
{
	NormalTangent tan[3];

//DS��@���Čv�Z
	NormalRecalculationEdge(Input);

//�ڃx�N�g���v�Z
	tan[0] = GetTangent(Input[0].Nor, (float3x3) wvpCb[Input[0].instanceID].world, Input[0].Tan);
	tan[1] = GetTangent(Input[1].Nor, (float3x3) wvpCb[Input[1].instanceID].world, Input[1].Tan);
	tan[2] = GetTangent(Input[2].Nor, (float3x3) wvpCb[Input[2].instanceID].world, Input[2].Tan);

	Stream(Input, triStream, tan);
}
//********Edge***********//
//****************************�O��DS,�m�[�}���}�b�v��**********************************************//
//********Smooth*********//
[maxvertexcount(3)]
void GS_Before_ds_NoNormalMap_Smooth(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)
{
	NormalTangent tan[3];
	GS_Mesh_INPUT recal[3];

//DS��@���Čv�Z
	NormalRecalculationEdge(Input);
	recal[0] = NormalRecalculationSmooth(Input[0]);
	recal[1] = NormalRecalculationSmooth(Input[1]);
	recal[2] = NormalRecalculationSmooth(Input[2]);

//�ڃx�N�g���v�Z����
	tan[0].normal = mul(recal[0].Nor, (float3x3) wvpCb[recal[0].instanceID].world);
	tan[1].normal = mul(recal[1].Nor, (float3x3) wvpCb[recal[1].instanceID].world);
	tan[2].normal = mul(recal[2].Nor, (float3x3) wvpCb[recal[2].instanceID].world);
	tan[0].tangent = float3(0.0f, 0.0f, 0.0f);
	tan[1].tangent = float3(0.0f, 0.0f, 0.0f);
	tan[2].tangent = float3(0.0f, 0.0f, 0.0f);

	Stream(recal, triStream, tan);
}
//********Smooth*********//
//********Edge***********//
[maxvertexcount(3)]
void GS_Before_ds_NoNormalMap_Edge(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)
{
	NormalTangent tan[3];

//DS��@���Čv�Z
	NormalRecalculationEdge(Input);

//�ڃx�N�g���v�Z����
	tan[0].normal = mul(Input[0].Nor, (float3x3) wvpCb[Input[0].instanceID].world);
	tan[1].normal = mul(Input[1].Nor, (float3x3) wvpCb[Input[1].instanceID].world);
	tan[2].normal = mul(Input[2].Nor, (float3x3) wvpCb[Input[2].instanceID].world);
	tan[0].tangent = float3(0.0f, 0.0f, 0.0f);
	tan[1].tangent = float3(0.0f, 0.0f, 0.0f);
	tan[2].tangent = float3(0.0f, 0.0f, 0.0f);

	Stream(Input, triStream, tan);
}
//********Edge***********//
//****************************�O��VS,�m�[�}���}�b�v�L**********************************************//
[maxvertexcount(3)]
void GS_Before_vs(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)
{
	NormalTangent tan[3];

//�ڃx�N�g���v�Z
	tan[0] = GetTangent(Input[0].Nor, (float3x3) wvpCb[Input[0].instanceID].world, Input[0].Tan);
	tan[1] = GetTangent(Input[1].Nor, (float3x3) wvpCb[Input[1].instanceID].world, Input[1].Tan);
	tan[2] = GetTangent(Input[2].Nor, (float3x3) wvpCb[Input[2].instanceID].world, Input[2].Tan);

	Stream(Input, triStream, tan);
}
//****************************�O��VS,�m�[�}���}�b�v��**********************************************//
[maxvertexcount(3)]
void GS_Before_vs_NoNormalMap(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <PS_INPUT> triStream)
{
	NormalTangent tan[3];

//�ڃx�N�g���v�Z����
	tan[0].normal = mul(Input[0].Nor, (float3x3) wvpCb[Input[0].instanceID].world);
	tan[1].normal = mul(Input[1].Nor, (float3x3) wvpCb[Input[1].instanceID].world);
	tan[2].normal = mul(Input[2].Nor, (float3x3) wvpCb[Input[2].instanceID].world);
	tan[0].tangent = float3(0.0f, 0.0f, 0.0f);
	tan[1].tangent = float3(0.0f, 0.0f, 0.0f);
	tan[2].tangent = float3(0.0f, 0.0f, 0.0f);

	Stream(Input, triStream, tan);
}
//**************************************�W�I���g���V�F�[�_�[********************************************************//