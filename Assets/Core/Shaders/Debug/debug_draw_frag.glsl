#version 460 core

#define kAntialiasing 2.0
#define MODE_POINTS 0
#define MODE_LINES  1

uniform int u_Mode;

in VertexData
{
	noperspective float m_edgeDistance;
	noperspective float m_size;
	smooth vec4 m_color;
} vData;

layout(location = 0) out vec4 fResult;

void main()
{
	fResult = vData.m_color;

	if (u_Mode == MODE_LINES)
	{
		fResult.a *= 1.0;
	}
	else if (u_Mode == MODE_POINTS)
	{
		float d = length(gl_PointCoord.xy - vec2(0.5));
		d = smoothstep(0.5, 0.5 - (kAntialiasing / vData.m_size), d);
		fResult.a *= d;
	}
	else
	{
		fResult = vec4(1, 0, 1, 1); // magenta, fallback
	}
}