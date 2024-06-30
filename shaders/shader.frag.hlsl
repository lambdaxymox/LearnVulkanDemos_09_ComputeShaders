Texture2D<float4> texSampler : register(t1, space0);
SamplerState texSamplerState : register(s1, space0);

struct PS_Input {
    float4 position : SV_POSITION;
    float3 fragColor : TEXCOORD0;
    float2 fragTexCoord: TEXCOORD1;
};

struct PS_Output {
    float4 outColor : SV_TARGET0;
};


PS_Output main(PS_Input input) {    
    float4 outFragColor = texSampler.Sample(texSamplerState, input.fragTexCoord);
    
    PS_Output output;
    output.outColor = outFragColor;
    
    return output;
}
