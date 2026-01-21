// Triangle shader for NVRHI demo
// Compile with: slangc triangle.slang -profile sm_6_0 -target dxil -entry vsMain -stage vertex -o triangle_vs.dxil
//               slangc triangle.slang -profile sm_6_0 -target dxil -entry psMain -stage fragment -o triangle_ps.dxil

// Vertex shader input
struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

// Vertex shader output / Pixel shader input
struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

// Vertex shader: Transform vertices and pass color to pixel shader
[shader("vertex")]
VSOutput vsMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}

// Pixel shader: Output the interpolated vertex color
[shader("fragment")]
float4 psMain(VSOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
