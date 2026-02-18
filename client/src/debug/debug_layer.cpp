#include "core/thread_context.hpp"
#ifdef DEBUG_BUILD

#    include "debug_layer.hpp"
#    include "global_client_state.hpp"

#    include <core/frame_context.hpp>
#    include <core/logger.hpp>
#    include <imgui.h>
#    include <implot.h>
#    include <memory/arena.hpp>
#    include <memory/arena_debug.hpp>
#    include <ui/icons.hpp>
#    include <utils/string.hpp>

// Catppuccin Mocha palette
constexpr ImU32 CAT_MAUVE = IM_COL32(203, 166, 247, 255);
constexpr ImU32 CAT_BLUE  = IM_COL32(137, 180, 250, 255);
constexpr ImU32 CAT_SKY   = IM_COL32(116, 199, 236, 255);
constexpr ImU32 CAT_TEAL  = IM_COL32(148, 226, 213, 255);
constexpr ImU32 CAT_GREEN = IM_COL32(166, 227, 161, 255);
constexpr ImU32 CAT_PEACH = IM_COL32(250, 179, 135, 255);
constexpr ImU32 CAT_PINK  = IM_COL32(245, 194, 231, 255);
constexpr ImU32 CAT_RED   = IM_COL32(243, 139, 168, 255);

// Allocation block color cycle (catppuccin)
constexpr ImU32 ALLOC_COLORS[] =
    {CAT_MAUVE, CAT_BLUE, CAT_SKY, CAT_TEAL, CAT_GREEN, CAT_PEACH, CAT_PINK};
constexpr u32 ALLOC_COLOR_COUNT =
    sizeof(ALLOC_COLORS) / sizeof(ALLOC_COLORS[0]);

// Other colors
constexpr ImU32 COLOR_FREE       = IM_COL32(60, 65, 75, 255);
constexpr ImU32 COLOR_PADDING    = IM_COL32(220, 140, 40, 200);
constexpr ImU32 COLOR_HEADER     = IM_COL32(88, 91, 112, 255);
constexpr ImU32 COLOR_BAR_BORDER = IM_COL32(80, 85, 95, 255);
constexpr ImU32 COLOR_BAR_BG     = IM_COL32(30, 30, 35, 255);

// Formats bytes as a human-readable ImGui text call.
// Uses ImGui's own formatting — no arena allocation needed.
INTERNAL_FUNC void
imgui_text_bytes(u64 bytes)
{
    if (bytes >= 1 * MiB)
        ImGui::Text("%.2f MiB", (f64)bytes / (f64)MiB);
    else if (bytes >= 1 * KiB)
        ImGui::Text("%.2f KiB", (f64)bytes / (f64)KiB);
    else
        ImGui::Text("%llu B", bytes);
}

INTERNAL_FUNC void
imgui_text_bytes_colored(ImVec4 color, u64 bytes)
{
    if (bytes >= 1 * MiB)
        ImGui::TextColored(color, "%.2f MiB", (f64)bytes / (f64)MiB);
    else if (bytes >= 1 * KiB)
        ImGui::TextColored(color, "%.2f KiB", (f64)bytes / (f64)KiB);
    else
        ImGui::TextColored(color, "%llu B", bytes);
}

INTERNAL_FUNC String
arena_display_name(const char *file, s32 line)
{
    String path     = STR(file);
    String filename = string_skip_last_slash(path);

    if (filename.size == 0)
        return STR_LIT("unknown");

    return filename;
}

// Tooltip helper for bytes — no arena allocation
INTERNAL_FUNC void
imgui_tooltip_bytes(const char *label, u64 bytes)
{
    if (bytes >= 1 * MiB)
        ImGui::Text("%s: %.2f MiB", label, (f64)bytes / (f64)MiB);
    else if (bytes >= 1 * KiB)
        ImGui::Text("%s: %.2f KiB", label, (f64)bytes / (f64)KiB);
    else
        ImGui::Text("%s: %llu B", label, bytes);
}

// Draw a horizontal disk-utilization bar for a single arena.
// Shows: header (muted), allocations (catppuccin colors cycling),
//        padding/fragmentation (orange), free (gray).
// Each section has a hover tooltip with size info.
// zoom_level controls horizontal scaling (1.0 = fit to width).
// scroll_x is the horizontal pan offset in normalized [0,1] range.
INTERNAL_FUNC void
render_arena_utilization_bar(Arena             *arena,
                             Arena_Debug_Entry *entry,
                             f32                bar_width,
                             f32                bar_height,
                             f32                zoom_level,
                             f32               *scroll_x)
{
    if (arena->committed_memory == 0)
        return;

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2      cursor    = ImGui::GetCursorScreenPos();

    f32 total    = (f32)arena->committed_memory;
    f32 zoomed_w = bar_width * zoom_level;
    f32 max_scroll =
        (zoomed_w > bar_width) ? (zoomed_w - bar_width) / zoomed_w : 0.0f;
    f32 pan    = (scroll_x ? *scroll_x : 0.0f);
    f32 pan_px = pan * zoomed_w;

    // Clip to visible region
    ImGui::PushClipRect(cursor,
                        ImVec2(cursor.x + bar_width, cursor.y + bar_height),
                        true);

    f32 origin_x = cursor.x - pan_px;

    // Background (full committed extent = gray/free)
    draw_list->AddRectFilled(ImVec2(origin_x, cursor.y),
                             ImVec2(origin_x + zoomed_w, cursor.y + bar_height),
                             COLOR_FREE);

    // Header block
    f32 header_x2 = origin_x + zoomed_w * ((f32)ARENA_HEADER_SIZE / total);
    draw_list->AddRectFilled(ImVec2(origin_x, cursor.y),
                             ImVec2(header_x2, cursor.y + bar_height),
                             COLOR_HEADER);

    // Draw allocation blocks and padding gaps
    for (u32 i = 0; i < entry->record_count; ++i)
    {
        Arena_Allocation_Record *rec = &entry->records[i];
        ImU32 alloc_color            = ALLOC_COLORS[i % ALLOC_COLOR_COUNT];

        // Padding block (fragmentation)
        if (rec->padding > 0)
        {
            f32 pad_x1 = origin_x +
                         zoomed_w * ((f32)(rec->offset - rec->padding) / total);
            f32 pad_x2 = origin_x + zoomed_w * ((f32)rec->offset / total);

            if ((pad_x2 - pad_x1) >= 1.0f)
            {
                draw_list->AddRectFilled(ImVec2(pad_x1, cursor.y),
                                         ImVec2(pad_x2, cursor.y + bar_height),
                                         COLOR_PADDING);
            }
        }

        // Allocation block
        f32 alloc_x1 = origin_x + zoomed_w * ((f32)rec->offset / total);
        f32 alloc_x2 =
            origin_x + zoomed_w * ((f32)(rec->offset + rec->size) / total);

        if ((alloc_x2 - alloc_x1) < 1.0f)
            alloc_x2 = alloc_x1 + 1.0f;

        draw_list->AddRectFilled(ImVec2(alloc_x1, cursor.y),
                                 ImVec2(alloc_x2, cursor.y + bar_height),
                                 alloc_color);
    }

    // Border
    draw_list->AddRect(cursor,
                       ImVec2(cursor.x + bar_width, cursor.y + bar_height),
                       COLOR_BAR_BORDER);

    ImGui::PopClipRect();

    // Reserve layout space
    ImGui::Dummy(ImVec2(bar_width, bar_height));

    // Handle zoom + pan on hover
    if (ImGui::IsItemHovered())
    {
        f32 wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f && scroll_x)
        {
            // Zoom centered on mouse position
            ImVec2 mouse  = ImGui::GetMousePos();
            f32    rel_x  = mouse.x - cursor.x;
            f32    norm_x = (rel_x + pan_px) / zoomed_w;

            f32 old_zoom = zoom_level;
            // Store new zoom in the state via the caller
            // (we modify the zoom through the detail panel's state)
            // For now, just handle tooltip — zoom is managed by caller
            (void)old_zoom;
            (void)norm_x;
        }

        // Tooltip — identify which section the mouse is over
        ImVec2 mouse        = ImGui::GetMousePos();
        f32    rel_x        = mouse.x - cursor.x;
        f32    hover_offset = ((rel_x + pan_px) / zoomed_w) * total;

        if (hover_offset >= 0.0f && hover_offset < total)
        {
            // Header region
            if (hover_offset < (f32)ARENA_HEADER_SIZE)
            {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(0.34f, 0.36f, 0.44f, 1.0f),
                                   "Arena Header");
                ImGui::Text("Size: %llu B", (u64)ARENA_HEADER_SIZE);
                ImGui::EndTooltip();
            }
            else
            {
                b8 found = false;
                for (u32 i = 0; i < entry->record_count; ++i)
                {
                    Arena_Allocation_Record *rec = &entry->records[i];
                    f32 block_start = (f32)(rec->offset - rec->padding);
                    f32 block_end   = (f32)(rec->offset + rec->size);

                    if (hover_offset >= block_start && hover_offset < block_end)
                    {
                        b8 on_padding = (rec->padding > 0) &&
                                        (hover_offset < (f32)rec->offset);

                        ImGui::BeginTooltip();
                        if (on_padding)
                        {
                            ImGui::TextColored(
                                ImVec4(0.86f, 0.55f, 0.16f, 1.0f),
                                "Alignment Padding");
                            imgui_tooltip_bytes("Size", rec->padding);
                            ImGui::Separator();
                            ImGui::TextDisabled("Before allocation #%u", i + 1);
                        }
                        else
                        {
                            String name =
                                arena_display_name(rec->file, rec->line);
                            ImGui::Text("Allocation #%u", i + 1);
                            ImGui::Separator();
                            ImGui::Text("Source:  %.*s:%d",
                                        (s32)(name).size, (name).buff ? (const char *)(name).buff : "",
                                        rec->line);
                            ImGui::Text("Offset:  0x%llX", rec->offset);
                            imgui_tooltip_bytes("Size", rec->size);

                            if (rec->padding > 0)
                            {
                                if (rec->padding >= 1 * KiB)
                                {
                                    ImGui::TextColored(
                                        ImVec4(0.86f, 0.55f, 0.16f, 1.0f),
                                        "Padding: %.2f KiB",
                                        (f64)rec->padding / (f64)KiB);
                                }
                                else
                                {
                                    ImGui::TextColored(
                                        ImVec4(0.86f, 0.55f, 0.16f, 1.0f),
                                        "Padding: %llu B",
                                        rec->padding);
                                }
                            }
                        }
                        ImGui::EndTooltip();
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    u64 free_bytes = arena->committed_memory - arena->offset;
                    ImGui::BeginTooltip();
                    ImGui::TextDisabled("Free (committed)");
                    imgui_tooltip_bytes("Size", free_bytes);
                    ImGui::EndTooltip();
                }
            }
        }
    }
}

INTERNAL_FUNC void
render_allocation_table(Arena_Debug_Entry *entry)
{
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_ScrollY |
                            ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_SizingStretchProp;

    f32 available_height =
        ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ChildRounding;
    f32 row_height       = ImGui::GetFrameHeightWithSpacing();
    f32 min_table_height = row_height * 6.0f; // header + 5 rows minimum
    f32 table_height     = MAX(available_height, min_table_height);

    if (ImGui::BeginTable("##AllocationTable",
                          5,
                          flags,
                          ImVec2(0.0f, table_height)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Offset",
                                ImGuiTableColumnFlags_WidthFixed,
                                90.0f);
        ImGui::TableSetupColumn("Size",
                                ImGuiTableColumnFlags_WidthFixed,
                                80.0f);
        ImGui::TableSetupColumn("Padding",
                                ImGuiTableColumnFlags_WidthFixed,
                                60.0f);
        ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (u32 i = 0; i < entry->record_count; ++i)
        {
            Arena_Allocation_Record *rec = &entry->records[i];

            ImGui::TableNextRow();

            ImU32  color   = ALLOC_COLORS[i % ALLOC_COLOR_COUNT];
            ImVec4 color_v = ImGui::ColorConvertU32ToFloat4(color);

            ImGui::TableNextColumn();
            ImGui::TextColored(color_v, "%u", i + 1);

            ImGui::TableNextColumn();
            ImGui::Text("0x%llX", rec->offset);

            ImGui::TableNextColumn();
            if (rec->size >= 1 * MiB)
                ImGui::Text("%.2f MiB", (f64)rec->size / (f64)MiB);
            else if (rec->size >= 1 * KiB)
                ImGui::Text("%.2f KiB", (f64)rec->size / (f64)KiB);
            else
                ImGui::Text("%llu B", rec->size);

            ImGui::TableNextColumn();
            if (rec->padding > 0)
            {
                ImGui::TextColored(ImVec4(0.86f, 0.55f, 0.16f, 1.0f),
                                   "%llu B",
                                   rec->padding);
            }
            else
            {
                ImGui::TextDisabled("0");
            }

            ImGui::TableNextColumn();
            String name = arena_display_name(rec->file, rec->line);
            ImGui::Text("%.*s:%d", (s32)(name).size, (name).buff ? (const char *)(name).buff : "", rec->line);
        }

        ImGui::EndTable();
    }
}

INTERNAL_FUNC void
render_arena_detail(Arena_Debug_Entry *entry, Debug_Layer_State *state)
{
    Arena *arena = entry->arena;

    u64 total_padding = 0;
    for (u32 i = 0; i < entry->record_count; ++i)
    {
        total_padding += entry->records[i].padding;
    }

    f32 utilization = 0.0f;
    if (arena->committed_memory > ARENA_HEADER_SIZE)
    {
        utilization = (f32)(arena->offset - ARENA_HEADER_SIZE) /
                      (f32)(arena->committed_memory - ARENA_HEADER_SIZE) *
                      100.0f;
    }

    // Header info
    String display =
        arena_display_name(arena->allocation_file, arena->allocation_line);

    ImGui::Text(ICON_FA_MICROCHIP " %.*s:%d",
                (s32)(display).size, (display).buff ? (const char *)(display).buff : "",
                arena->allocation_line);
    ImGui::Separator();

    // Metrics row
    ImGui::Columns(4, nullptr, false);

    ImGui::Text("Reserved");
    imgui_text_bytes_colored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                             arena->reserved_memory);
    ImGui::NextColumn();

    ImGui::Text("Committed");
    imgui_text_bytes_colored(ImVec4(0.54f, 0.71f, 0.98f, 1.0f),
                             arena->committed_memory);
    ImGui::NextColumn();

    ImGui::Text("Used");
    imgui_text_bytes_colored(ImVec4(0.65f, 0.89f, 0.63f, 1.0f), arena->offset);
    ImGui::NextColumn();

    ImGui::Text("Waste (padding)");
    imgui_text_bytes_colored(ImVec4(0.86f, 0.55f, 0.16f, 1.0f), total_padding);
    ImGui::NextColumn();
    ImGui::Columns(1);

    ImGui::Spacing();

    // Reserved vs committed overview bar
    ImGui::Text("Reserved Extent");
    {
        f32         bar_w = ImGui::GetContentRegionAvail().x;
        f32         bar_h = 18.0f;
        ImDrawList *dl    = ImGui::GetWindowDrawList();
        ImVec2      p     = ImGui::GetCursorScreenPos();

        dl->AddRectFilled(p, ImVec2(p.x + bar_w, p.y + bar_h), COLOR_BAR_BG);

        if (arena->reserved_memory > 0)
        {
            f32 committed_w = bar_w * ((f32)arena->committed_memory /
                                       (f32)arena->reserved_memory);
            dl->AddRectFilled(p,
                              ImVec2(p.x + committed_w, p.y + bar_h),
                              IM_COL32(137, 180, 250, 100));

            f32 used_w =
                bar_w * ((f32)arena->offset / (f32)arena->reserved_memory);
            dl->AddRectFilled(p,
                              ImVec2(p.x + used_w, p.y + bar_h),
                              IM_COL32(166, 227, 161, 150));
        }
        dl->AddRect(p, ImVec2(p.x + bar_w, p.y + bar_h), COLOR_BAR_BORDER);

        ImGui::Dummy(ImVec2(bar_w, bar_h));

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            imgui_tooltip_bytes("Reserved ", arena->reserved_memory);
            imgui_tooltip_bytes("Committed", arena->committed_memory);
            imgui_tooltip_bytes("Used     ", arena->offset);
            ImGui::EndTooltip();
        }
    }

    ImGui::Spacing();

    // Detailed memory map (disk-utilization style) with zoom
    ImGui::Text(
        "Memory Map - %.1f%c utilized  (scroll to zoom, "
        "drag to pan)",
        utilization,
        '%');

    // Legend
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();

        ImDrawList *tip_draw = ImGui::GetWindowDrawList();
        f32         sq       = 10.0f;

        auto draw_legend_entry = [&](ImU32 color, const char *label)
        {
            ImVec2 p = ImGui::GetCursorScreenPos();
            tip_draw->AddRectFilled(p, ImVec2(p.x + sq, p.y + sq), color);
            ImGui::Dummy(ImVec2(sq, sq));
            ImGui::SameLine();
            ImGui::Text("%s", label);
        };

        draw_legend_entry(COLOR_HEADER, "Arena Header");
        draw_legend_entry(CAT_MAUVE, "Allocations (cycling colors)");
        draw_legend_entry(COLOR_PADDING, "Alignment Padding");
        draw_legend_entry(COLOR_FREE, "Free (committed)");

        ImGui::EndTooltip();
    }

    // Zoom + pan controls
    f32 bar_width  = ImGui::GetContentRegionAvail().x;
    f32 bar_height = 32.0f;

    // Handle zoom with mouse wheel before drawing
    ImVec2 bar_cursor = ImGui::GetCursorScreenPos();
    ImVec2 mouse      = ImGui::GetMousePos();

    b8 mouse_over_bar =
        (mouse.x >= bar_cursor.x && mouse.x <= bar_cursor.x + bar_width &&
         mouse.y >= bar_cursor.y && mouse.y <= bar_cursor.y + bar_height);

    if (mouse_over_bar)
    {
        f32 wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
        {
            f32 zoomed_w   = bar_width * state->zoom_level;
            f32 pan_px     = state->scroll_x * zoomed_w;
            f32 rel_x      = mouse.x - bar_cursor.x;
            f32 norm_mouse = (rel_x + pan_px) / zoomed_w;

            f32 old_zoom = state->zoom_level;
            state->zoom_level *= (wheel > 0.0f) ? 1.2f : (1.0f / 1.2f);
            state->zoom_level = CLAMP(state->zoom_level, 1.0f, 2000.0f);

            // Adjust pan to keep the mouse-pointed region stationary
            f32 new_zoomed_w = bar_width * state->zoom_level;
            f32 new_pan_px   = norm_mouse * new_zoomed_w - rel_x;
            f32 max_pan_px   = new_zoomed_w - bar_width;

            if (max_pan_px > 0.0f)
                state->scroll_x = CLAMP(new_pan_px / new_zoomed_w,
                                        0.0f,
                                        max_pan_px / new_zoomed_w);
            else
                state->scroll_x = 0.0f;
        }

        // Middle-mouse drag to pan
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
        {
            f32 zoomed_w = bar_width * state->zoom_level;
            f32 dx       = ImGui::GetIO().MouseDelta.x;

            state->scroll_x -= dx / zoomed_w;

            f32 max_pan     = (zoomed_w > bar_width)
                                  ? (zoomed_w - bar_width) / zoomed_w
                                  : 0.0f;
            state->scroll_x = CLAMP(state->scroll_x, 0.0f, max_pan);
        }
    }
    else if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    {
        // Detail child has NoScrollWithMouse set, so provide manual wheel
        // scrolling when not interacting with the zoomable memory bar.
        f32 wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
        {
            f32 scroll_step = 5.0f * ImGui::GetFontSize();
            f32 max_step    = ImGui::GetWindowHeight() * 0.67f;
            if (scroll_step > max_step)
            {
                scroll_step = max_step;
            }

            ImGui::SetScrollY(ImGui::GetScrollY() - wheel * scroll_step);
        }
    }

    if (state->zoom_level > 1.01f)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("  %.0fx", state->zoom_level);
    }

    render_arena_utilization_bar(arena,
                                 entry,
                                 bar_width,
                                 bar_height,
                                 state->zoom_level,
                                 &state->scroll_x);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Allocation size distribution (ImPlot bar chart)
    if (entry->record_count > 0)
    {
        ImGui::Text("Allocation Size Distribution");

        constexpr u32 BUCKET_COUNT = 6;
        const char   *bucket_labels[BUCKET_COUNT] =
            {"1-64B", "65-256B", "257B-1K", "1K-4K", "4K-64K", ">64K"};
        f64 bucket_counts[BUCKET_COUNT] = {};

        for (u32 i = 0; i < entry->record_count; ++i)
        {
            u64 sz = entry->records[i].size;
            if (sz <= 64)
                bucket_counts[0]++;
            else if (sz <= 256)
                bucket_counts[1]++;
            else if (sz <= 1 * KiB)
                bucket_counts[2]++;
            else if (sz <= 4 * KiB)
                bucket_counts[3]++;
            else if (sz <= 64 * KiB)
                bucket_counts[4]++;
            else
                bucket_counts[5]++;
        }

        f64 positions[BUCKET_COUNT];
        for (u32 i = 0; i < BUCKET_COUNT; ++i)
            positions[i] = (f64)i;

        if (ImPlot::BeginPlot("##SizeDist",
                              ImVec2(-1, 150),
                              ImPlotFlags_NoMouseText))
        {
            ImPlot::SetupAxes("Size Range",
                              "Count",
                              ImPlotAxisFlags_AutoFit,
                              ImPlotAxisFlags_AutoFit);

            ImPlot::SetupAxisTicks(ImAxis_X1,
                                   positions,
                                   BUCKET_COUNT,
                                   bucket_labels);

            ImPlot::SetNextFillStyle(ImVec4(0.80f, 0.65f, 0.97f, 0.85f));

            ImPlot::PlotBars("Allocations", bucket_counts, BUCKET_COUNT, 0.6);

            ImPlot::EndPlot();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    // Allocation table
    ImGui::Text("Allocations (%u records)", entry->record_count);

    render_allocation_table(entry);
}

void
debug_layer_on_attach(void *state_ptr)
{
    Debug_Layer_State *state = (Debug_Layer_State *)state_ptr;

    state->selected_arena_index = -1;
    state->zoom_level           = 1.0f;
    state->scroll_x             = 0.0f;

    CLIENT_INFO("Debug layer attached");
}

void
debug_layer_on_detach(void *state_ptr)
{
    CLIENT_INFO("Debug layer detached");
}

b8
debug_layer_on_update(void *state_ptr, void *global_state, Frame_Context *ctx)
{
    (void)state_ptr;
    (void)global_state;
    (void)ctx;
    return true;
}

b8
debug_layer_on_render(void *layer_state, void *global_state, Frame_Context *ctx)
{
    Debug_Layer_State   *l_state = (Debug_Layer_State *)layer_state;
    Global_Client_State *g_state = (Global_Client_State *)global_state;

    if (g_state->is_debug_layer_visible)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.80f);
        ImGui::Begin(ICON_FA_BUG " Memory Inspector",
                     &g_state->is_debug_layer_visible,
                     ImGuiWindowFlags_NoDocking);

        Arena_Debug_Registry *registry = arena_debug_get_registry();
        if (!registry)
        {
            ImGui::Text("Debug registry not initialized.");
            ImGui::End();
            ImGui::PopStyleVar();
            return true;
        }

        // Overview: per-arena utilization bars (disk-usage style)
        if (registry->active_count > 0)
        {
            ImGui::Text(ICON_FA_DATABASE " Arena Overview (%u arenas)",
                        registry->active_count);
            ImGui::Separator();

            f32 overview_bar_width = ImGui::GetContentRegionAvail().x;

            for (u32 i = 0; i < ARENA_DEBUG_MAX_ARENAS; ++i)
            {
                Arena_Debug_Entry *entry = &registry->entries[i];
                if (!entry->active)
                    continue;

                Arena *arena = entry->arena;

                String name = arena_display_name(arena->allocation_file,
                                                 arena->allocation_line);

                f32 pct = 0.0f;
                if (arena->committed_memory > 0)
                {
                    pct = (f32)arena->offset / (f32)arena->committed_memory *
                          100.0f;
                }

                f64 measurement_unit = (f64)MiB;

                // Arena label with utilization %
                if (arena->offset < MiB)
                    measurement_unit = (f64)KiB;

                String display_measurement_unit = (measurement_unit == (f64)MiB)
                                                      ? STR_LIT("MiB")
                                                      : STR_LIT("KiB");

                auto scratch_arena = scratch_begin(&arena, 1);

                String arena_details =
                    string_fmt(scratch_arena.arena,
                            "%.*s:%d  —  %.2f %.*s / %.2f %.*s  (%.0f%c)",
                            (s32)(name).size, (name).buff ? (const char *)(name).buff : "",
                            arena->allocation_line,
                            arena->offset / measurement_unit,
                            (s32)(display_measurement_unit).size, (display_measurement_unit).buff ? (const char *)(display_measurement_unit).buff : "",
                            arena->committed_memory / measurement_unit,
                            (s32)(display_measurement_unit).size, (display_measurement_unit).buff ? (const char *)(display_measurement_unit).buff : "",
                            pct,
                            '%');

                const char *arena_details_begin = arena_details.buff
                                                      ? (const char *)arena_details.buff
                                                      : "";
                ImGui::TextUnformatted(arena_details_begin,
                                       arena_details_begin +
                                           (arena_details.buff ? arena_details.size
                                                              : 0));

                scratch_end(scratch_arena);

                // Mini utilization bar (no zoom for overview)
                f32 no_scroll = 0.0f;
                render_arena_utilization_bar(arena,
                                             entry,
                                             overview_bar_width,
                                             14.0f,
                                             1.0f,
                                             &no_scroll);

                ImGui::Spacing();
            }

            ImGui::Separator();
            ImGui::Spacing();
        }

        // Left panel: arena list
        f32 list_width = 260.0f;

        ImGui::BeginChild("##ArenaList",
                          ImVec2(list_width, 0),
                          ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);

        ImGui::Text(ICON_FA_DATABASE " Arenas (%u)", registry->active_count);
        ImGui::Separator();

        for (u32 i = 0; i < ARENA_DEBUG_MAX_ARENAS; ++i)
        {
            Arena_Debug_Entry *entry = &registry->entries[i];
            if (!entry->active)
                continue;

            Arena *arena = entry->arena;

            String name = arena_display_name(arena->allocation_file,
                                             arena->allocation_line);

            b8 is_selected = (l_state->selected_arena_index == (s32)i);

            ImGui::PushID((s32)i);

            const char *file  = arena->allocation_file;
            const char *slash = file;
            for (const char *p = file; *p; ++p)
            {
                if (*p == '/' || *p == '\\')
                    slash = p + 1;
            }

            if (ImGui::Selectable(slash, is_selected))
            {
                l_state->selected_arena_index = (s32)i;
                l_state->zoom_level           = 1.0f;
                l_state->scroll_x             = 0.0f;
            }

            auto scratch = scratch_begin(&arena, 1);

            f64 measurement_unit = (f64)MiB;

            // Arena label with utilization %
            if (arena->offset < MiB)
                measurement_unit = (f64)KiB;

            String display_measurement_unit = (measurement_unit == (f64)MiB)
                                                  ? STR_LIT("MiB")
                                                  : STR_LIT("KiB");

            String arena_summary =
                string_fmt(scratch.arena,
                        "  %.2f %.*s / %.2f %.*s  (%u allocs)",
                        arena->offset / measurement_unit,
                        (s32)(display_measurement_unit).size, (display_measurement_unit).buff ? (const char *)(display_measurement_unit).buff : "",
                        arena->committed_memory / measurement_unit,
                        (s32)(display_measurement_unit).size, (display_measurement_unit).buff ? (const char *)(display_measurement_unit).buff : "",
                        entry->record_count);

            const char *arena_summary_begin =
                arena_summary.buff ? (const char *)arena_summary.buff : "";
            ImGui::TextUnformatted(arena_summary_begin,
                                   arena_summary_begin +
                                       (arena_summary.buff ? arena_summary.size
                                                          : 0));

            scratch_end(scratch);

            ImGui::PopID();
        }

        ImGui::EndChild();

        // Right panel: detail view
        ImGui::SameLine();

        ImGui::BeginChild("##ArenaDetail",
                          ImVec2(0, 0),
                          ImGuiChildFlags_Border,
                          ImGuiWindowFlags_NoScrollWithMouse);

        if (l_state->selected_arena_index >= 0 &&
            l_state->selected_arena_index < (s32)ARENA_DEBUG_MAX_ARENAS)
        {
            Arena_Debug_Entry *selected =
                &registry->entries[l_state->selected_arena_index];

            if (selected->active)
            {
                render_arena_detail(selected, l_state);
            }
            else
            {
                l_state->selected_arena_index = -1;
                ImGui::TextDisabled("Select an arena from the list.");
            }
        }
        else
        {
            ImGui::TextDisabled("Select an arena from the list.");
        }

        ImGui::EndChild();

        ImGui::End();
        ImGui::PopStyleVar();
    }

    return true;
}

UI_Layer
create_debug_layer(Debug_Layer_State *state)
{
    UI_Layer layer  = {};
    layer.on_attach = debug_layer_on_attach;
    layer.on_detach = debug_layer_on_detach;
    layer.on_update = debug_layer_on_update;
    layer.on_render = debug_layer_on_render;
    layer.state     = state;
    return layer;
}

#endif // DEBUG_BUILD
