// Triangle shader for NVRHI demo (HLSL version)
// Alternative to Slang version, compile with DXC:
//   dxc -T vs_6_0 -E vsMain triangle.hlsl -Fo triangle_vs.dxil
//   dxc -T ps_6_0 -E psMain triangle.hlsl -Fo triangle_ps.dxil

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
VSOutput vsMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}

// Pixel shader: Output the interpolated vertex color
float4 psMain(VSOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
