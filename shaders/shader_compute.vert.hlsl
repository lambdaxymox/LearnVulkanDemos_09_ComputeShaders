struct VS_Input {
    float2 position : TEXCOORD0;
    float4 color : TEXCOORD1;
};

struct VS_Output {
    float4 position : SV_POSITION;
    float3 fragColor : TEXCOORD0;
    float pointSize : PSIZE;
};


VS_Output main(VS_Input input) {
    float4 outPosition = float4(input.position.xy, 1.0, 1.0);
    float3 outFragColor = input.color.rgb;
    float outPointSize = 14.0;

    VS_Output output;
    output.position = outPosition;
    output.fragColor = outFragColor;
    output.pointSize = outPointSize;

    return output;
}

