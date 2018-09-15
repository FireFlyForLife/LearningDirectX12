struct Mat
{
    matrix ModelViewProjectionMatrix;
};

ConstantBuffer<Mat> MatCB : register(b0);
// const matrix MVP : register(b0);

struct ColoredLinePoint
{
    float4 Color    : COLOR;
    //--------------------------------16 bytes
    float3 Position : POSITION;
    //Total = 16+12 = 28 bytes
};

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput main(ColoredLinePoint IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul( MatCB.ModelViewProjectionMatrix, float4(IN.Position, 1.0f));
    OUT.Color = IN.Color;

    return OUT;
}
