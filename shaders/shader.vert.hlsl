struct VS_Output {
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
};

VS_Output main(uint VertexIndex : SV_VertexID) {
    float2 positions[3] = {
        float2(0.0, -0.5),
        float2(0.5, 0.5),
        float2(-0.5, 0.5)
    };

    float3 colors[3] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };

    VS_Output output = (VS_Output)0;
    output.Position = float4(positions[VertexIndex], 0.0, 1.0);
    output.Color = float4(colors[VertexIndex], 1.0);

    return output;
}
