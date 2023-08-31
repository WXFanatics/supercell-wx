#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform float uMapDistance;

in VertexData
{
   int  threshold;
   vec2 texCoord;
   vec4 color;
} gsIn[];

smooth out vec2 texCoord;
smooth out vec4 color;

void main()
{
   if (gsIn[0].threshold <= 0 ||            // If Threshold: 0 was specified, no threshold
       gsIn[0].threshold >= uMapDistance || // If Threshold is above current map distance
       gsIn[0].threshold >= 999)            // If Threshold: 999 was specified (or greater), no threshold
   {
      gl_Position = gl_in[0].gl_Position;
      texCoord    = gsIn[0].texCoord;
      color       = gsIn[0].color;
      EmitVertex();

      gl_Position = gl_in[1].gl_Position;
      texCoord    = gsIn[1].texCoord;
      color       = gsIn[1].color;

      EmitVertex();
      gl_Position = gl_in[2].gl_Position;
      texCoord    = gsIn[2].texCoord;
      color       = gsIn[2].color;

      EmitVertex();
      EndPrimitive();
   }
}