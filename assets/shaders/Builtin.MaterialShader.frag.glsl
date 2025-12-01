#version 450

layout(location = 0) out vec4 out_colour;

layout(set = 1, binding = 0) uniform local_uniform_object {
    vec4 diffuse_colour;
} object_ubo;

// Samplers
layout(set = 1, binding = 1) uniform sampler2D diffuse_sampler;

// Input from vertex shader
layout(location = 0) flat in int in_mode;

// Data transfer object
layout(location = 1) in struct dto {
    vec2 texture_coordinate;
} in_dto;

void main() {
    out_colour = object_ubo.diffuse_colour * texture(diffuse_sampler, in_dto.texture_coordinate);
}
