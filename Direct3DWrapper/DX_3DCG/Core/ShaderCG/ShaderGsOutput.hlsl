///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     ShaderGsOutput.hlsl                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

struct DXR_INPUT
{
	float3 Pos : POSITION;
	float3 Nor : NORMAL;
	float3 Tan : TANGENT;
	float2 Tex0 : TEXCOORD0;
	float2 Tex1 : TEXCOORD1;
};
//**************************************前がVS*************************************************//
[maxvertexcount(3)]
void GS_Before_vs(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <DXR_INPUT> triStream)
{
	DXR_INPUT p = (DXR_INPUT) 0;

	p.Pos = Input[0].Pos.xyz;
	p.Nor = Input[0].Nor;
	p.Tan = Input[0].Tan;
	p.Tex0 = Input[0].Tex0;
	p.Tex1 = Input[0].Tex1;
	triStream.Append(p);
	p.Pos = Input[1].Pos.xyz;
	p.Nor = Input[1].Nor;
	p.Tan = Input[1].Tan;
	p.Tex0 = Input[1].Tex0;
	p.Tex1 = Input[1].Tex1;
	triStream.Append(p);
	p.Pos = Input[2].Pos.xyz;
	p.Nor = Input[2].Nor;
	p.Tan = Input[2].Tan;
	p.Tex0 = Input[2].Tex0;
	p.Tex1 = Input[2].Tex1;
	triStream.Append(p);
	triStream.RestartStrip();
}
//**************************************前がVS*************************************************//

//**************************************前がDS*************************************************//
//********Smooth*********//
[maxvertexcount(3)]
void GS_Before_ds_Smooth(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <DXR_INPUT> triStream)
{
	DXR_INPUT p = (DXR_INPUT) 0;
	GS_Mesh_INPUT recal[3];

//DS後法線再計算
	NormalRecalculationEdge(Input);
	recal[0] = NormalRecalculationSmooth(Input[0]);
	recal[1] = NormalRecalculationSmooth(Input[1]);
	recal[2] = NormalRecalculationSmooth(Input[2]);

	p.Pos = recal[0].Pos.xyz;
	p.Nor = recal[0].Nor;
	p.Tan = recal[0].Tan;
	p.Tex0 = recal[0].Tex0;
	p.Tex1 = recal[0].Tex1;
	triStream.Append(p);
	p.Pos = recal[1].Pos.xyz;
	p.Nor = recal[1].Nor;
	p.Tan = recal[1].Tan;
	p.Tex0 = recal[1].Tex0;
	p.Tex1 = recal[1].Tex1;
	triStream.Append(p);
	p.Pos = recal[2].Pos.xyz;
	p.Nor = recal[2].Nor;
	p.Tan = recal[2].Tan;
	p.Tex0 = recal[2].Tex0;
	p.Tex1 = recal[2].Tex1;
	triStream.Append(p);
	triStream.RestartStrip();
}
//********Smooth*********//
//********Edge***********//
[maxvertexcount(3)]
void GS_Before_ds_Edge(triangle GS_Mesh_INPUT Input[3], inout TriangleStream <DXR_INPUT> triStream)
{
	DXR_INPUT p = (DXR_INPUT) 0;

//DS後法線再計算
	NormalRecalculationEdge(Input);

	p.Pos = Input[0].Pos.xyz;
	p.Nor = Input[0].Nor;
	p.Tan = Input[0].Tan;
	p.Tex0 = Input[0].Tex0;
	p.Tex1 = Input[0].Tex1;
	triStream.Append(p);
	p.Pos = Input[1].Pos.xyz;
	p.Nor = Input[1].Nor;
	p.Tan = Input[1].Tan;
	p.Tex0 = Input[1].Tex0;
	p.Tex1 = Input[1].Tex1;
	triStream.Append(p);
	p.Pos = Input[2].Pos.xyz;
	p.Nor = Input[2].Nor;
	p.Tan = Input[2].Tan;
	p.Tex0 = Input[2].Tex0;
	p.Tex1 = Input[2].Tex1;
	triStream.Append(p);
	triStream.RestartStrip();
}
//********Edge***********//
//**************************************前がDS*************************************************//
