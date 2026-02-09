#include "editor_layer.hpp"

#include "components/viewport_component.hpp"
#include "global_client_state.hpp"

#include <core/frame_context.hpp>
#include <core/logger.hpp>
#include <events/events.hpp>
#include <imgui.h>
#include <math/math.hpp>
#include <memory/memory.hpp>
#include <renderer/renderer_frontend.hpp>
#include <ui/icons.hpp>

// Static pointer to editor state for event callbacks
internal_var Editor_Layer_State *editor_state = nullptr;

INTERNAL_FUNC void render_statistics_window(Editor_Layer_State *state,
                                            f32                 delta_time);
INTERNAL_FUNC void render_signal_analyzer(Editor_Layer_State *state,
                                          f32                 delta_time);

// Event callbacks
INTERNAL_FUNC b8 on_mouse_wheel(const Event *event);
INTERNAL_FUNC b8 on_mouse_moved(const Event *event);

void
editor_layer_on_attach(void *state_ptr)
{
    Editor_Layer_State *state = (Editor_Layer_State *)state_ptr;
    editor_state              = state;

    viewport_component_on_attach(state);

    // Initialize metrics state
    state->fps             = 0.0f;
    state->frame_time_ms   = 0.0f;
    state->fps_accumulator = 0.0f;
    state->fps_frame_count = 0;

    // Signal analyzer
    state->signal_time = 0.0f;

    // Register event handlers
    events_register_callback(Event_Type::MOUSE_WHEEL_SCROLLED,
                             on_mouse_wheel,
                             Event_Priority::HIGHEST);

    events_register_callback(Event_Type::MOUSE_MOVED,
                             on_mouse_moved,
                             Event_Priority::HIGHEST);

    CLIENT_INFO("Editor layer attached");
}

void
editor_layer_on_detach(void *state_ptr)
{
    events_unregister_callback(Event_Type::MOUSE_WHEEL_SCROLLED,
                               on_mouse_wheel);
    events_unregister_callback(Event_Type::MOUSE_MOVED, on_mouse_moved);

    editor_state = nullptr;

    CLIENT_INFO("Editor layer detached");
}

b8
editor_layer_on_update(void *state_ptr, void *global_state, Frame_Context *ctx)
{
    Editor_Layer_State *l_state = (Editor_Layer_State *)state_ptr;
    viewport_component_on_update(l_state, ctx);
    (void)global_state;
    return true;
}

b8
editor_layer_on_render(void          *layer_state,
                       void          *global_state,
                       Frame_Context *ctx)
{
    Editor_Layer_State  *l_state = (Editor_Layer_State *)layer_state;
    Global_Client_State *g_state = (Global_Client_State *)global_state;

    viewport_component_on_render(l_state, g_state, ctx->delta_t);
    render_statistics_window(l_state, ctx->delta_t);

    render_signal_analyzer(l_state, ctx->delta_t);

    if (g_state->is_imgui_demo_visible)
    {
        ImGui::ShowDemoWindow(&g_state->is_imgui_demo_visible);
    }

    if (g_state->is_implot_demo_visible)
    {
        ImPlot::ShowDemoWindow(&g_state->is_implot_demo_visible);
    }

    return true;
}

INTERNAL_FUNC b8
on_mouse_wheel(const Event *event)
{
    return viewport_component_on_mouse_wheel(editor_state, event);
}

INTERNAL_FUNC b8
on_mouse_moved(const Event *event)
{
    return viewport_component_on_mouse_moved(editor_state, event);
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
    if (state->signal_time >= 2.0f)
    {
        state->signal_time -= 2.0f;
    }

    ImGui::Begin(ICON_FA_BOLT " Signal Analyzer");

    ImGui::Text("Electrical Signal Analysis");
    ImGui::Separator();

    // Generate sample data for visualization
    constexpr int     sample_count = 512;
    local_persist f32 time_data[sample_count];
    local_persist f32 voltage_signal[sample_count];
    local_persist f32 current_signal[sample_count];
    local_persist f32 power_signal[sample_count];
    local_persist f32 pwm_signal[sample_count];

    f32 base_time = state->signal_time;

    f32 base_freq = 0.5f;

    for (int i = 0; i < sample_count; ++i)
    {
        f32 t        = (f32)i / (f32)sample_count * 2.0f;
        time_data[i] = t;

        f32 phase         = (base_time + t) * base_freq * math::PI_2;
        voltage_signal[i] = 100.0f * math_sin(phase);
        current_signal[i] = 100.0f * math_cos(phase);
        power_signal[i]   = 50.0f + 50.0f * math_sin(phase * 0.5f);
        pwm_signal[i]     = 2.5f + 2.5f * math_sin(phase * 2.0f);
    }

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
