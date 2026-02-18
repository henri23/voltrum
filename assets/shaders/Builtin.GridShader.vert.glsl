#version 450

layout(set = 0, binding = 0) uniform grid_uniform_object {
    mat4 projection;
    mat4 inv_view;
    vec4 grid_color;
    float grid_spacing_world;
    float point_size_px;
    float zoom_px_per_world;
} grid_ubo;

layout(location = 0) out vec2 out_world_pos;

void main()
{
    // Fullscreen triangle via gl_VertexIndex (no vertex buffer needed)
    // Vertices: (-1,-1), (3,-1), (-1,3) - covers entire viewport
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );

    vec2 ndc = positions[gl_VertexIndex];

    // Build inverse(view * projection) from CPU-provided inv_view + projection.
    mat4 inv_projection      = inverse(grid_ubo.projection);
    mat4 inv_view_projection = grid_ubo.inv_view * inv_projection;
    vec4 world_h             = inv_view_projection * vec4(ndc, 0.0, 1.0);
    out_world_pos            = world_h.xy / world_h.w;

    gl_Position = vec4(ndc, 0.0, 1.0);
}
