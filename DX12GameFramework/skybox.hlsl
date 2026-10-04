TextureCube texCube : register(t0);
SamplerState smp : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
    matrix view;
    matrix proj;
}

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 localPos : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    result.localPos = input.position;
    
    // view行列を 3x3 にキャストし、平行移動成分を切り捨てる。
    // カメラの回転のみが適用され、空が常にカメラの中心に追従する。
    float3 viewPos3 = mul(input.position, (float3x3) view);
    
    float4 viewPos = float4(viewPos3, 1.0f);
    result.position = mul(viewPos, proj);
    
    // 射影変換後の Z を W と同じ値に強制する (Z/W = 1.0)。
    // 深度バッファにおいて常に一番奥(1.0)に描画させるため。
    result.position.z = result.position.w;
    
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // TextureCube は 3D方向ベクトルを用いてサンプリングする。
    return texCube.Sample(smp, input.localPos);

}