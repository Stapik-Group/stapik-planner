#pragma once

#include "../../../core/model/DayPlan.hpp"

#include <array>

namespace ScheduleWeekdays
{
    inline constexpr int COUNT = 7;
    inline constexpr int COLUMN_SPACING = 8;

    inline constexpr std::array<Weekday, COUNT> ALL = {
        Weekday::MONDAY, Weekday::TUESDAY, Weekday::WEDNESDAY, Weekday::THURSDAY,
        Weekday::FRIDAY, Weekday::SATURDAY, Weekday::SUNDAY
    };

    inline constexpr std::array<const char*, COUNT> KEYS = {
        "day.mon", "day.tue", "day.wed", "day.thu", "day.fri", "day.sat", "day.sun"
    };

    inline constexpr std::array<const char*, COUNT> FULL_KEYS = {
        "day.full.mon", "day.full.tue", "day.full.wed", "day.full.thu", "day.full.fri", "day.full.sat", "day.full.sun"
    };
}
