///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           Shader2D.hlsl                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

Texture2D g_texColor : register(t0, space0);
SamplerState g_samLinear : register(s0, space0);

struct WVPCB_2d
{
    matrix world;
    float4 AddObjColor;
    float4 pXpYmXmY;
};

ConstantBuffer<WVPCB_2d> wvpCb[] : register(b0, space1);

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Col : COLOR;
	float2 Tex : TEXCOORD;
};

//**************************基本色頂点**********************************//
VS_OUTPUT VSBaseColor(float4 Pos : POSITION, float4 Col : COLOR, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
	
    matrix world = wvpCb[instanceID].world;
    float4 addCol = wvpCb[instanceID].AddObjColor;
	
    output.Pos = mul(Pos, world);
    output.Col = Col + addCol;
    
    return output;
}
//**************************基本色頂点**********************************//

//**************************基本色ピクセル******************************//
float4 PSBaseColor(VS_OUTPUT input) : SV_Target
{
    return input.Col;
}
//**************************基本色ピクセル******************************//

//**************************テクスチャ頂点******************************//
VS_OUTPUT VSTextureColor(float4 Pos : POSITION, float4 Col : COLOR, float2 Tex : TEXCOORD, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
	
    matrix world = wvpCb[instanceID].world;
    float4 addCol = wvpCb[instanceID].AddObjColor;
    float4 pXpYmXmY = wvpCb[instanceID].pXpYmXmY;
	
    output.Pos = mul(Pos, world);
    output.Col = Col + addCol;
    output.Tex.x = Tex.x * pXpYmXmY.x + pXpYmXmY.x * pXpYmXmY.z;
    output.Tex.y = Tex.y * pXpYmXmY.y + pXpYmXmY.y * pXpYmXmY.w;
    
    return output;
}
//**************************テクスチャ頂点******************************//

//**************************テクスチャピクセル**************************//
float4 PSTextureColor(VS_OUTPUT input) : SV_Target
{
    return input.Col * g_texColor.Sample(g_samLinear, input.Tex);
}
//**************************テクスチャピクセル**************************//
