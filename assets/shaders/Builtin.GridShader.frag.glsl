#version 450

layout(location = 0) in vec2 in_world_pos;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform grid_uniform_object {
    mat4 projection;
    mat4 inv_view;
    vec4 grid_color;
    float grid_spacing_world;
    float point_size_px;
    float zoom_px_per_world;
} grid_ubo;

void main()
{
    // Distance to nearest grid point in screen-space pixels.
    vec2  nearest = round(in_world_pos / grid_ubo.grid_spacing_world) * grid_ubo.grid_spacing_world;
    vec2  diff    = in_world_pos - nearest;
    float dist_px = length(diff) * grid_ubo.zoom_px_per_world;

    // Anti-aliased dot.
    float aa_width = max(0.25, grid_ubo.point_size_px * 0.35);
    float dot_mask = 1.0 - smoothstep(
        grid_ubo.point_size_px - aa_width,
        grid_ubo.point_size_px + aa_width,
        dist_px);

    // Fade out only when spacing gets extremely dense on screen.
    float screen_spacing = grid_ubo.grid_spacing_world * grid_ubo.zoom_px_per_world;
    float density_fade   = smoothstep(1.0, 4.0, screen_spacing);

    float alpha = dot_mask * density_fade * grid_ubo.grid_color.a;

    // Grid pipeline uses premultiplied-alpha blending (srcColor = ONE),
    // so RGB must be multiplied by alpha here.
    out_color = vec4(grid_ubo.grid_color.rgb * alpha, alpha);
}
