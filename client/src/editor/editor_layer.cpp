#include "editor_layer.hpp"

#include "input/input.hpp"
#include "input/input_codes.hpp"
#include <core/logger.hpp>
#include <imgui.h>
#include <math/math.hpp>
#include <memory/memory.hpp>
#include <renderer/renderer_frontend.hpp>
#include <ui/icons.hpp>

// Static pointer to editor state for demo window control from menu callback
internal_var Editor_Layer_State *editor_state = nullptr;

INTERNAL_FUNC void viewport_camera_initialize(Viewport_Camera *camera,
                                              vec3 position = {0, 0, 10.0f});
INTERNAL_FUNC void viewport_camera_recalculate_view(Viewport_Camera *camera);
INTERNAL_FUNC void viewport_camera_rotate_yaw(Viewport_Camera *camera,
                                              f32              amount);
INTERNAL_FUNC void viewport_camera_rotate_pitch(Viewport_Camera *camera,
                                                f32              amount);
INTERNAL_FUNC b8   viewport_camera_update(Viewport_Camera *camera,
                                          f32              delta_time,
                                          b8               viewport_active);

INTERNAL_FUNC void render_viewport_window(Editor_Layer_State *state,
                                          f32                 delta_time);
INTERNAL_FUNC void render_statistics_window(Editor_Layer_State *state,
                                            f32                 delta_time);
INTERNAL_FUNC void render_signal_analyzer(Editor_Layer_State *state,
                                          f32                 delta_time);

void
editor_layer_on_attach(UI_Layer *self)
{
    Editor_Layer_State *state = (Editor_Layer_State *)self->state;
    editor_state              = state;

    viewport_camera_initialize(&state->camera, {0, 0, 10.0f});
    viewport_camera_recalculate_view(&state->camera);
    renderer_set_view(state->camera.view_matrix);

    state->viewport_focused = false;
    state->viewport_hovered = false;

    // Initialize metrics state
    state->fps             = 0.0f;
    state->frame_time_ms   = 0.0f;
    state->fps_accumulator = 0.0f;
    state->fps_frame_count = 0;

    // Demo windows
    state->show_demo_window        = true;
    state->show_implot_demo_window = true;

    // Signal analyzer
    state->show_signal_analyzer = true;
    state->signal_time          = 0.0f;

    u32 width  = 0;
    u32 height = 0;
    renderer_get_viewport_size(&width, &height);
    state->viewport_size      = {(f32)width, (f32)height};
    state->last_viewport_size = state->viewport_size;

    CLIENT_INFO("Editor layer attached");
}

void
editor_layer_on_detach(UI_Layer *self)
{
    editor_state = nullptr;

    CLIENT_INFO("Editor layer detached");
}

b8
editor_layer_on_update(UI_Layer *self, f32 delta_time)
{
    Editor_Layer_State *state = (Editor_Layer_State *)self->state;

    b8 viewport_active = state->viewport_hovered;
    // b8 viewport_active = state->viewport_focused || state->viewport_hovered;

    b8 camera_moved =
        viewport_camera_update(&state->camera, delta_time, viewport_active);

    if (camera_moved)
    {
        renderer_set_view(state->camera.view_matrix);
    }

    return true;
}

b8
editor_layer_on_render(UI_Layer *self, f32 delta_time)
{
    Editor_Layer_State *state = (Editor_Layer_State *)self->state;

    render_viewport_window(state, delta_time);
    render_statistics_window(state, delta_time);

    if (state->show_signal_analyzer)
    {
        render_signal_analyzer(state, delta_time);
    }

    if (state->show_demo_window)
    {
        ImGui::ShowDemoWindow(&state->show_demo_window);
    }

    if (state->show_implot_demo_window)
    {
        ImPlot::ShowDemoWindow(&state->show_implot_demo_window);
    }

    return true;
}

INTERNAL_FUNC void
viewport_camera_initialize(Viewport_Camera *camera, vec3 position)
{
    camera->position      = position;
    camera->euler_angles  = vec3_zero();
    camera->camera_matrix = mat4_translation(camera->position);
    camera->view_matrix   = mat4_inv(camera->camera_matrix);
    camera->view_dirty    = true;
}

INTERNAL_FUNC void
viewport_camera_recalculate_view(Viewport_Camera *camera)
{
    if (camera->view_dirty)
    {
        mat4 rotation         = mat4_euler_xyz(camera->euler_angles.x,
                                       camera->euler_angles.y,
                                       camera->euler_angles.z);
        mat4 translation      = mat4_translation(camera->position);
        camera->camera_matrix = rotation * translation;
        camera->view_matrix   = mat4_inv(camera->camera_matrix);
        camera->view_dirty    = false;
    }
}

INTERNAL_FUNC void
viewport_camera_rotate_yaw(Viewport_Camera *camera, f32 amount)
{
    camera->euler_angles.y += amount;
    camera->view_dirty = true;
}

INTERNAL_FUNC void
viewport_camera_rotate_pitch(Viewport_Camera *camera, f32 amount)
{
    camera->euler_angles.x += amount;
    f32 limit              = deg_to_rad(89.0f);
    camera->euler_angles.x = CLAMP(camera->euler_angles.x, -limit, limit);
    camera->view_dirty     = true;
}

INTERNAL_FUNC b8
viewport_camera_update(Viewport_Camera *camera,
                       f32              delta_time,
                       b8               viewport_active)
{
    b8 camera_moved = false;

    if (viewport_active)
    {
        f32 rotation_velocity = 1.5f;
        f32 rotation_delta    = rotation_velocity * delta_time;

        if (input_is_key_pressed(Key_Code::A) ||
            input_is_key_pressed(Key_Code::LEFT))
        {
            viewport_camera_rotate_yaw(camera, rotation_delta);
            camera_moved = true;
        }
        if (input_is_key_pressed(Key_Code::D) ||
            input_is_key_pressed(Key_Code::RIGHT))
        {
            viewport_camera_rotate_yaw(camera, -rotation_delta);
            camera_moved = true;
        }
        if (input_is_key_pressed(Key_Code::UP))
        {
            viewport_camera_rotate_pitch(camera, rotation_delta);
            camera_moved = true;
        }
        if (input_is_key_pressed(Key_Code::DOWN))
        {
            viewport_camera_rotate_pitch(camera, -rotation_delta);
            camera_moved = true;
        }

        f32  movement_speed = 5.0f;
        vec3 velocity       = vec3_zero();

        if (input_is_key_pressed(Key_Code::W))
        {
            vec3 forward = mat4_forward(camera->camera_matrix);
            velocity     = velocity + forward;
        }
        if (input_is_key_pressed(Key_Code::S))
        {
            vec3 forward = mat4_backward(camera->camera_matrix);
            velocity     = velocity + forward;
        }
        if (input_is_key_pressed(Key_Code::Q))
        {
            vec3 left = mat4_left(camera->camera_matrix);
            velocity  = velocity + left;
        }
        if (input_is_key_pressed(Key_Code::E))
        {
            vec3 right = mat4_right(camera->camera_matrix);
            velocity   = velocity + right;
        }
        if (input_is_key_pressed(Key_Code::SPACE))
        {
            velocity.y += 1.0f;
        }
        if (input_is_key_pressed(Key_Code::X))
        {
            velocity.y -= 1.0f;
        }

        vec3 zero = vec3_zero();
        if (!vec3_are_equal(zero, velocity, 0.0002f))
        {
            vec3_norm(&velocity);
            f32 move_delta = movement_speed * delta_time;
            camera->position.x += velocity.x * move_delta;
            camera->position.y += velocity.y * move_delta;
            camera->position.z += velocity.z * move_delta;
            camera->view_dirty = true;
            camera_moved       = true;
        }
    }

    if (camera->view_dirty)
    {
        viewport_camera_recalculate_view(camera);
        camera_moved = true;
    }

    return camera_moved;
}

INTERNAL_FUNC void
render_viewport_window(Editor_Layer_State *state, f32 delta_time)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(ICON_FA_EXPAND " Viewport");
    ImGui::PopStyleVar();

    state->viewport_focused = ImGui::IsWindowFocused();
    state->viewport_hovered = ImGui::IsWindowHovered();

    ImVec2 content_size  = ImGui::GetContentRegionAvail();
    state->viewport_size = {content_size.x, content_size.y};

    if (state->viewport_size.x != state->last_viewport_size.x ||
        state->viewport_size.y != state->last_viewport_size.y)
    {

        u32 width =
            (u32)(state->viewport_size.x < 1.0f ? 1.0f
                                                : state->viewport_size.x);
        u32 height =
            (u32)(state->viewport_size.y < 1.0f ? 1.0f
                                                : state->viewport_size.y);

        CLIENT_DEBUG("Viewport window resized to %ux%u", width, height);

        renderer_resize_viewport(width, height);

        state->last_viewport_size = state->viewport_size;
    }

    // Render viewport and get the current frame's descriptor
    renderer_render_viewport();

    ImGui::Image(renderer_get_rendered_viewport(),
                 content_size,
                 ImVec2(0, 0),
                 ImVec2(1, 1));

    ImGui::End();
}

INTERNAL_FUNC void
render_statistics_window(Editor_Layer_State *state, f32 delta_time)
{
    // Update FPS calculation - average over ~0.5 seconds for stability
    state->fps_accumulator += delta_time;
    state->fps_frame_count++;

    if (state->fps_accumulator >= 0.5f)
    {
        state->fps = (f32)state->fps_frame_count / state->fps_accumulator;
        state->frame_time_ms =
            (state->fps_accumulator / state->fps_frame_count) * 1000.0f;
        state->fps_accumulator = 0.0f;
        state->fps_frame_count = 0;
    }

    ImGui::Begin(ICON_FA_CHART_LINE " Statistics");

    ImGui::Text("FPS: %.1f", state->fps);
    ImGui::Text("Frame Time: %.2f ms", state->frame_time_ms);
    ImGui::Separator();
    ImGui::Text("Allocations: %llu", memory_get_allocations_count());

    ImGui::End();
}

INTERNAL_FUNC void
render_signal_analyzer(Editor_Layer_State *state, f32 delta_time)
{
    state->signal_time += delta_time;
    // Wrap signal_time to make the simulation periodic (2 second period)
    // This matches the PWM duty cycle variation period: sin(base_time * PI)
    if (state->signal_time >= 2.0f)
    {
        state->signal_time -= 2.0f;
    }

    ImGui::Begin(ICON_FA_BOLT " Signal Analyzer", &state->show_signal_analyzer);

    ImGui::Text("Electrical Signal Analysis");
    ImGui::Separator();

    // Generate sample data for visualization
    constexpr int sample_count = 512;
    static f32    time_data[sample_count];
    static f32    voltage_signal[sample_count];
    static f32    current_signal[sample_count];
    static f32    power_signal[sample_count];
    static f32    pwm_signal[sample_count];

    f32 base_time = state->signal_time;

    // Slow frequencies for easy visual tracking (1 full cycle per 2 seconds)
    f32 base_freq = 0.5f; // 0.5 Hz - completes 1 cycle in 2 seconds

    for (int i = 0; i < sample_count; ++i)
    {
        // Time window shows 2 seconds of data
        f32 t        = (f32)i / (f32)sample_count * 2.0f;
        time_data[i] = t; // Time in seconds

        // Smooth sine wave (easy to track)
        f32 phase         = (base_time + t) * base_freq * math::PI_2;
        voltage_signal[i] = 100.0f * math_sin(phase);

        // Cosine wave (90 degree phase shift)
        current_signal[i] = 100.0f * math_cos(phase);

        // Smooth power envelope (always positive, slow variation)
        power_signal[i] = 50.0f + 50.0f * math_sin(phase * 0.5f);

        // Smooth triangle-like wave using sine approximation
        pwm_signal[i] = 2.5f + 2.5f * math_sin(phase * 2.0f);
    }

    // Voltage and Current waveforms
    if (ImPlot::BeginPlot("##VoltageCurrentPlot", ImVec2(-1, 200)))
    {
        ImPlot::SetupAxes("Time (s)", "Amplitude");
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, 2, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -120, 120, ImGuiCond_Always);
        ImPlot::SetupLegend(ImPlotLocation_NorthEast);

        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.9f, 0.4f, 0.1f, 1.0f));
        ImPlot::PlotLine("Sine", time_data, voltage_signal, sample_count);
        ImPlot::PopStyleColor();

        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.2f, 0.7f, 0.9f, 1.0f));
        ImPlot::PlotLine("Cosine", time_data, current_signal, sample_count);
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }

    // Power waveform
    if (ImPlot::BeginPlot("##PowerPlot", ImVec2(-1, 150)))
    {
        ImPlot::SetupAxes("Time (s)", "Value");
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, 2, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -10, 110, ImGuiCond_Always);

        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.3f, 0.9f, 0.4f, 1.0f));
        ImPlot::SetNextFillStyle(ImVec4(0.3f, 0.9f, 0.4f, 0.25f));
        ImPlot::PlotShaded("Slow Wave", time_data, power_signal, sample_count);
        ImPlot::PlotLine("Slow Wave", time_data, power_signal, sample_count);
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }

    // PWM Control Signal
    if (ImPlot::BeginPlot("##PWMPlot", ImVec2(-1, 100)))
    {
        ImPlot::SetupAxes("Time (s)", "Value");
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, 2, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -0.5, 6, ImGuiCond_Always);

        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.9f, 0.2f, 0.6f, 1.0f));
        ImPlot::PlotLine("Fast Wave", time_data, pwm_signal, sample_count);
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                       "Smooth periodic signals (2 second cycle)");

    ImGui::End();
}

UI_Layer
create_editor_layer(Editor_Layer_State *state)
{
    UI_Layer layer  = {};
    layer.on_attach = editor_layer_on_attach;
    layer.on_detach = editor_layer_on_detach;
    layer.on_update = editor_layer_on_update;
    layer.on_render = editor_layer_on_render;
    layer.state     = state;
    return layer;
}

void
editor_toggle_demo_window()
{
    if (editor_state)
    {
        editor_state->show_demo_window = !editor_state->show_demo_window;
    }
}

b8
editor_is_demo_window_visible()
{
    return editor_state ? editor_state->show_demo_window : false;
}

void
editor_toggle_implot_demo_window()
{
    if (editor_state)
    {
        editor_state->show_implot_demo_window =
            !editor_state->show_implot_demo_window;
    }
}

b8
editor_is_implot_demo_window_visible()
{
    return editor_state ? editor_state->show_implot_demo_window : false;
}

void
editor_toggle_signal_analyzer()
{
    if (editor_state)
    {
        editor_state->show_signal_analyzer =
            !editor_state->show_signal_analyzer;
    }
}

b8
editor_is_signal_analyzer_visible()
{
    return editor_state ? editor_state->show_signal_analyzer : false;
}
