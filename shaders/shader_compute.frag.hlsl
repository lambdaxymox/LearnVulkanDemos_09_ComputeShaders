struct PS_Input {
    float4 position : SV_POSITION;
    float3 fragColor : TEXCOORD0;
    float pointSize : PSIZE;
};

struct PS_Output {
    float4 outFragColor : SV_TARGET0;
};


PS_Output main(PS_Input input) {
    float2 coord = input.position.xy - float2(0.5, 0.5);
    float4 outFragColor = float4(input.fragColor, 0.5 - length(coord));

    PS_Output output;
    output.outFragColor = outFragColor;

    return output;
}
