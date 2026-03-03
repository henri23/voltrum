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

void
pick_step_125_blended(
    float ratio,
    out float step_a,
    out float step_b,
    out float blend_t)
{
    if (ratio <= 1.0)
    {
        step_a  = 1.0;
        step_b  = 1.0;
        blend_t = 0.0;
        return;
    }

    float ratio_exp   = floor(log(ratio) / log(10.0));
    float ratio_scale = ratio / pow(10.0, ratio_exp);
    float decade      = pow(10.0, ratio_exp);

    float low_step  = 1.0;
    float high_step = 2.0;

    if (ratio_scale < 2.0)
    {
        low_step  = 1.0;
        high_step = 2.0;
    }
    else if (ratio_scale < 5.0)
    {
        low_step  = 2.0;
        high_step = 5.0;
    }
    else
    {
        low_step  = 5.0;
        high_step = 10.0;
    }

    step_a = low_step * decade;
    step_b = high_step * decade;

    float transition_start = high_step * 0.72;
    float transition_end   = high_step * 1.02;
    blend_t = smoothstep(transition_start, transition_end, ratio_scale);
}

float
grid_dot_mask(
    vec2 world_pos,
    float spacing_world,
    float zoom_px_per_world,
    float dot_size_px,
    float aa_px)
{
    vec2  nearest = round(world_pos / spacing_world) * spacing_world;
    vec2  diff    = world_pos - nearest;
    float dist_px = length(diff) * zoom_px_per_world;

    return 1.0 - smoothstep(
        dot_size_px - aa_px,
        dot_size_px + aa_px,
        dist_px);
}

void main()
{
    float base_screen_spacing =
        max(
            grid_ubo.grid_spacing_world * grid_ubo.zoom_px_per_world,
            0.0001);

    // Preserve user spacing for snapping, while rendering a coarser visual grid
    // when dots become too dense on screen.
    float min_visual_spacing_px = 14.0;
    float ratio                 = min_visual_spacing_px / base_screen_spacing;
    float density_step_a        = 1.0;
    float density_step_b        = 1.0;
    float density_blend_t       = 0.0;
    pick_step_125_blended(
        ratio,
        density_step_a,
        density_step_b,
        density_blend_t);

    float visual_spacing_world_a =
        max(grid_ubo.grid_spacing_world * density_step_a, 0.000001);
    float visual_spacing_world_b =
        max(grid_ubo.grid_spacing_world * density_step_b, 0.000001);
    float visual_spacing_px_a    = base_screen_spacing * density_step_a;
    float visual_spacing_px_b    = base_screen_spacing * density_step_b;

    float major_factor = 5.0;

    float minor_size_px = clamp(
        grid_ubo.point_size_px * 0.75,
        0.55,
        1.10);
    float major_size_px = clamp(
        grid_ubo.point_size_px * 1.45,
        0.90,
        1.80);

    float minor_aa = max(0.20, minor_size_px * 0.55);
    float major_aa = max(0.25, major_size_px * 0.55);

    float minor_mask_a = grid_dot_mask(
        in_world_pos,
        visual_spacing_world_a,
        grid_ubo.zoom_px_per_world,
        minor_size_px,
        minor_aa);
    float major_mask_a = grid_dot_mask(
        in_world_pos,
        visual_spacing_world_a * major_factor,
        grid_ubo.zoom_px_per_world,
        major_size_px,
        major_aa);

    float minor_mask_b = grid_dot_mask(
        in_world_pos,
        visual_spacing_world_b,
        grid_ubo.zoom_px_per_world,
        minor_size_px,
        minor_aa);
    float major_mask_b = grid_dot_mask(
        in_world_pos,
        visual_spacing_world_b * major_factor,
        grid_ubo.zoom_px_per_world,
        major_size_px,
        major_aa);

    float layer_alpha_a = max(
        minor_mask_a * 0.45,
        major_mask_a * 0.90);
    float layer_alpha_b = max(
        minor_mask_b * 0.45,
        major_mask_b * 0.90);
    float dot_alpha = mix(layer_alpha_a, layer_alpha_b, density_blend_t);

    float density_fade_a = smoothstep(4.0, 9.0, visual_spacing_px_a);
    float density_fade_b = smoothstep(4.0, 9.0, visual_spacing_px_b);
    float density_fade   = mix(density_fade_a, density_fade_b, density_blend_t);

    float alpha = dot_alpha * density_fade * grid_ubo.grid_color.a;

    // Grid pipeline uses premultiplied-alpha blending (srcColor = ONE),
    // so RGB must be multiplied by alpha here.
    out_color = vec4(grid_ubo.grid_color.rgb * alpha, alpha);
}
