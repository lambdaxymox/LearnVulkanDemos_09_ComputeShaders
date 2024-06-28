struct PS_Input {
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
};

float4 main(PS_Input pin) : SV_TARGET {
    return pin.Color;
}
