Texture2D tex : register(t0);
SamplerState smp : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
    matrix view;
    matrix proj;
}

cbuffer ObjectConstantBuffer : register(b1)
{
    matrix world;
    matrix boneTransforms[256];
    float4 materialColors[256];
}

cbuffer MaterialIndexBuffer : register(b2)
{
    uint materialIndex;
}

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    uint4 boneIDs : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHT;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    float4 pos = float4(input.position, 1.0f);
    float4 localPos =
            mul(pos, boneTransforms[input.boneIDs.x]) * input.boneWeights.x +
            mul(pos, boneTransforms[input.boneIDs.y]) * input.boneWeights.y +
            mul(pos, boneTransforms[input.boneIDs.z]) * input.boneWeights.z +
            mul(pos, boneTransforms[input.boneIDs.w]) * input.boneWeights.w;
    
    float4 worldPos = mul(localPos, world);
    float4 viewPos = mul(worldPos, view);
    result.position = mul(viewPos, proj);

    result.color = input.color;
    result.uv = input.uv;
    result.normal = input.normal;
    
    return result;

}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 texColor = tex.Sample(smp, input.uv);    
    float4 baseColor = materialColors[materialIndex];    
    return input.color * texColor * baseColor;
    
    //normal
    //float3 normalColor = (input.normal + 1.0f) * 0.5f;
    //return float4(normalColor, 1.0f);
}