#pragma once

#include "defines.hpp"

#include <imgui.h>
#include <math.h>

namespace ui
{
namespace anim
{

    // Exponential decay toward target value.
    inline f32
    exp_decay_to(f32 current, f32 target, f32 sharpness, f32 delta_time)
    {
        if (delta_time <= 0.0f)
        {
            return current;
        }

        if (sharpness <= 0.0f)
        {
            return target;
        }

        const f32 alpha = 1.0f - expf(-sharpness * delta_time);
        return current + ((target - current) * alpha);
    }

    // Persistent bool-track animation using ImGui storage and an ID.
    inline f32
    track_bool(ImGuiStorage *storage,
               ImGuiID       id,
               b8            active,
               f32           delta_time,
               f32           rise_sharpness = 22.0f,
               f32           fall_sharpness = 16.0f,
               f32           min_value      = 0.0f,
               f32           max_value      = 1.0f)
    {
        if (!storage)
        {
            return active ? max_value : min_value;
        }

        f32 *value = storage->GetFloatRef(id, min_value);
        const f32 target = active ? max_value : min_value;
        const f32 sharpness = active ? rise_sharpness : fall_sharpness;
        *value = exp_decay_to(*value, target, sharpness, delta_time);
        return *value;
    }

    // Popup/window alpha animation built on exponential decay.
    inline f32
    track_popup_alpha(ImGuiStorage *storage,
                      ImGuiID       id,
                      b8            is_open,
                      f32           delta_time,
                      f32           open_sharpness  = 26.0f,
                      f32           close_sharpness = 18.0f)
    {
        return track_bool(storage,
                          id,
                          is_open,
                          delta_time,
                          open_sharpness,
                          close_sharpness,
                          0.0f,
                          1.0f);
    }

} // namespace anim
} // namespace ui
