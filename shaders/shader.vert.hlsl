struct VS_Input {
    float3 position : TEXCOORD0;
    float3 color : TEXCOORD1;
    float2 texCoord: TEXCOORD2;
};

struct VS_Output {
    float4 position : SV_POSITION;
    float3 fragColor : TEXCOORD0;
    float2 fragTexCoord: TEXCOORD1;
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
    float4 outPosition = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.position, 1.0f))));
    float3 outFragColor = input.color;
    float2 outFragTexCoord = input.texCoord;
    
    VS_Output output;
    output.position = outPosition;
    output.fragColor = outFragColor;
    output.fragTexCoord = outFragTexCoord;
    
    return output;
}
