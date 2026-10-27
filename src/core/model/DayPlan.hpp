#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "Activity.hpp"

enum class Weekday
{
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    SUNDAY
};

struct DayPlan
{
    static constexpr int LOAD_LIMIT = 100;

    Weekday weekday;
    std::vector<std::optional<Activity>> slots;

    explicit DayPlan(const Weekday day, const int slotsCount) :
        weekday(day),
        slots(static_cast<size_t>(slotsCount), std::nullopt) {}

    [[nodiscard]] bool operator==(const DayPlan&) const = default;

    [[nodiscard]] bool isEmpty() const
    {
        return std::ranges::none_of(slots, [](const std::optional<Activity>& slot) { return slot.has_value(); });
    }

    [[nodiscard]] int totalLoad() const
    {
        int load = 0;
        for (const auto& slot : slots)
        {
            if (slot.has_value())
            {
                load += Activity::difficultyLoad(slot->difficulty);
            }
        }

        return load;
    }

    static std::string serializeWeekday(const Weekday weekday)
    {
        switch (weekday)
        {
            using enum Weekday;
            case MONDAY: return "monday";
            case TUESDAY: return "tuesday";
            case WEDNESDAY: return "wednesday";
            case THURSDAY: return "thursday";
            case FRIDAY: return "friday";
            case SATURDAY: return "saturday";
            case SUNDAY: return "sunday";
        }
        return "monday";
    }

    static std::optional<Weekday> deserializeWeekday(const std::string& str)
    {
        if (str == "monday") return Weekday::MONDAY;
        if (str == "tuesday") return Weekday::TUESDAY;
        if (str == "wednesday") return Weekday::WEDNESDAY;
        if (str == "thursday") return Weekday::THURSDAY;
        if (str == "friday") return Weekday::FRIDAY;
        if (str == "saturday") return Weekday::SATURDAY;
        if (str == "sunday") return Weekday::SUNDAY;
        return std::nullopt;
    }
};