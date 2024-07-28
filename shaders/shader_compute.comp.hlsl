struct Particle {
    float2 position;
    float2 velocity;
    float4 color;
};

struct CS_ParameterUBO {
    float deltaTime;
};


StructuredBuffer<Particle> inParticleBuffer : register(t1, space0);

RWStructuredBuffer<Particle> outParticleBuffer : register(u2, space0);

cbuffer ParameterUBO : register(b0, space0) {
    CS_ParameterUBO ubo;
};


[numthreads(256, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
    uint index = threadID.x;
    
    Particle inParticle = inParticleBuffer[index];

    Particle outParticle;
    outParticle.position = inParticle.position + (inParticle.velocity * ubo.deltaTime);
    outParticle.velocity = inParticle.velocity;
    outParticle.color = inParticle.color;

    if ((outParticle.position.x <= -1.0) || (outParticle.position.x >= 1.0)) {
        outParticle.velocity.x = -outParticle.velocity.x;
    }

    if ((outParticle.position.y <= -1.0) || (outParticle.position.y >= 1.0)) {
        outParticle.velocity.y = -outParticle.velocity.y;
    }

    outParticleBuffer[index] = outParticle;
}
