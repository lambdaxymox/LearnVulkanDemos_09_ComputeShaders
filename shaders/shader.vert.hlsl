struct VS_Input {
    float2 position : TEXCOORD0;
    float3 color : TEXCOORD1;
};

struct VS_Output {
    float4 position : SV_POSITION;
    float3 fragColor : TEXCOORD0;
};

struct VS_InputConstants {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

cbuffer ubo : register(b0) {
    VS_InputConstants ubo;
}


VS_Output main(VS_Input input) {
    float4 outPosition = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.position, 0.0f, 1.0f))));
    float3 outFragColor = input.color;
    
    VS_Output output;
    output.position = outPosition;
    output.fragColor = outFragColor;
    
    return output;
}
