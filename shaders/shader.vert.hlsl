struct VS_Input {
    float2 position : TEXCOORD0;
    float3 color : TEXCOORD1;
};

struct VS_Output {
    float4 position : SV_POSITION;
    float3 fragColor : TEXCOORD0;
};


VS_Output main(VS_Input input) {
    float4 outPosition = float4(input.position, 0.0f, 1.0f);
    float3 outFragColor = input.color;
    
    VS_Output output;
    output.position = outPosition;
    output.fragColor = outFragColor;
    
    return output;
}
