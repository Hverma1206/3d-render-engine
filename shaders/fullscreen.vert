#version 410 core
// Big-triangle trick: 3 vertices cover the entire screen with no buffer data.
// gl_VertexID 0,1,2 → positions (-1,-1), (3,-1), (-1,3).
out vec2 vUV;
void main()
{
    const vec2 pos[3] = vec2[](vec2(-1.0,-1.0), vec2(3.0,-1.0), vec2(-1.0,3.0));
    const vec2 uv[3]  = vec2[](vec2( 0.0, 0.0), vec2(2.0, 0.0), vec2( 0.0, 2.0));
    vUV         = uv[gl_VertexID];
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
}
