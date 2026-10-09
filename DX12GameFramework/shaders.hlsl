Texture2D tex : register(t0);
SamplerState smp : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
    matrix view;
    matrix proj;
    float4 lightDir;
    float4 lightColor;
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
    
    // 法線もボーンの動きに合わせる
    float4 norm = float4(input.normal, 0.0f);
    float4 localNormal =
            mul(norm, boneTransforms[input.boneIDs.x]) * input.boneWeights.x +
            mul(norm, boneTransforms[input.boneIDs.y]) * input.boneWeights.y +
            mul(norm, boneTransforms[input.boneIDs.z]) * input.boneWeights.z +
            mul(norm, boneTransforms[input.boneIDs.w]) * input.boneWeights.w;
    
    float4 worldNormal = mul(localNormal, world);
    
    float4 viewPos = mul(worldPos, view);
    result.position = mul(viewPos, proj);

    result.color = input.color;
    result.uv = input.uv;
    result.normal = normalize(worldNormal.xyz);
    
    return result;

}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 texColor = tex.Sample(smp, input.uv);
    float4 baseColor = materialColors[materialIndex];
    float4 albedo = input.color * texColor * baseColor;
    
    // --- Lambert
    // 面から高原へ向かうベクトルを作成
    float3 L = -lightDir.xyz;
    
    // 法線ベクトル
    float3 N = normalize(input.normal);
    
    // ライトの方向ベクトルと法線ベクトルで内積を取る
    // 光が当たらない裏面はマイナスになるため0にする
    float NdotL = max(0.0f, dot(N, L));
    
    // 光の強さ（色）を掛けてディフューズ光を求める
    float3 diffuseLight = lightColor.rgb * NdotL;
    
    
    // --- Ambient
    // ディフューズのみだと影の部分が真っ黒になるため全体に弱く当たる環境光を足す
    float3 ambientLight = float3(0.2f, 0.2f, 0.2f);
    
    // 色の合成
    // 物体の色 x (直接光 + 環境光)
    float3 finalColor = albedo.rgb * (diffuseLight + ambientLight);

    return float4(finalColor, albedo.a);

}