#version 460 core

#define kAntialiasing 2.0
#define MODE_POINTS 0
#define MODE_LINES  1

uniform mat4 u_ViewProjMatrix;
uniform int u_Mode; // 0 = POINTS, 1 = LINES

layout(location = 0) in vec4 aPositionSize; // xyz = position, w = size
layout(location = 1) in vec4 aColor;        // RGBA

out VertexData
{
	noperspective float m_edgeDistance;
	noperspective float m_size;
	smooth vec4 m_color;
} vData;

void main()
{
	vData.m_color = aColor.abgr; 
	vData.m_size = max(aPositionSize.w, kAntialiasing);
    vData.m_color.a *= smoothstep(0.0, 1.0, aPositionSize.w / kAntialiasing);

	if (u_Mode == MODE_POINTS)
	{
		gl_PointSize = vData.m_size;
		vData.m_edgeDistance = 0.0;
	}
	gl_Position = u_ViewProjMatrix * vec4(aPositionSize.xyz, 1.0);
}