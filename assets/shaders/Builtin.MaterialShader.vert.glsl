#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texture_coordinate;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
    mat4 view;
} global_ubo;

layout(push_constant) uniform push_constant {
    // Can be guaranteed only at 128 bytes
    mat4 model; // 64 bytes
} u_push_constants;

layout(location = 0) out int out_mode;

// We are passing the texture coordinates in the vertex shader
// however we are using them in the fragment shader so we need
// a transfer object to pass through these coordinates to the
// next shader
layout(location = 1) out struct data_transfer_object {
    vec2 texture_coordinate;
} out_dto;

void main() {
    out_mode = 0;
    out_dto.texture_coordinate = in_texture_coordinate;
    gl_Position = global_ubo.projection *
            global_ubo.view *
            u_push_constants.model *
            vec4(in_position, 1.0);
}
