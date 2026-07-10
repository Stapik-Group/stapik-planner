#pragma once

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
    Weekday weekday;
    std::vector<std::optional<Activity>> slots;

    explicit DayPlan(const Weekday day, const int slotsCount) :
        weekday(day),
        slots(static_cast<size_t>(slotsCount), std::nullopt) {}

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

    [[nodiscard]] bool canAddActivity(const Difficulty difficulty) const
    {
        return totalLoad() + Activity::difficultyLoad(difficulty) <= 100;
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

    static Weekday deserializeWeekday(const std::string& str)
    {
        if (str == "tuesday") return Weekday::TUESDAY;
        if (str == "wednesday") return Weekday::WEDNESDAY;
        if (str == "thursday") return Weekday::THURSDAY;
        if (str == "friday") return Weekday::FRIDAY;
        if (str == "saturday") return Weekday::SATURDAY;
        if (str == "sunday") return Weekday::SUNDAY;
        return Weekday::MONDAY;
    }
};