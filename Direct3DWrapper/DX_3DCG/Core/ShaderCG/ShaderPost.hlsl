///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderPost                                                  //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOut VSMain(uint id : SV_VertexID)
{
    VSOut o;

    float2 pos[3] =
    {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0)
    };

    float2 uv[3] =
    {
        float2(0.0, 1.0),
        float2(0.0, -1.0),
        float2(2.0, 1.0)
    };

    o.pos = float4(pos[id], 0.0, 1.0);
    o.uv = uv[id];

    return o;
}

Texture2D<float4> gHDR : register(t0, space0);
SamplerState gSmp : register(s0, space0);

cbuffer ToneMapCB : register(b0, space0)
{
    float Exposure;
};

float4 PSMain(VSOut input) : SV_TARGET
{
    float3 color = gHDR.Sample(gSmp, input.uv).rgb;

    color *= Exposure;

    //Reinhard Tone Mapping
    color = color / (1.0 + color);

    //Gammaï‚ê≥ÅisRGBãﬂéóÅj
    color = pow(color, 1.0 / 2.2);
    
    return float4(color, 1.0);
}