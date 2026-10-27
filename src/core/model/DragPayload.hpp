#pragma once

#include "DayPlan.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

struct DragPayload
{
    enum class Kind
    {
        Activity,
        Slot
    };

    Kind kind = Kind::Activity;
    std::string activityId;
    Weekday day = Weekday::MONDAY;
    std::size_t slot = 0;
    bool copy = false;

    [[nodiscard]] static DragPayload forActivity(std::string activityId);
    [[nodiscard]] static DragPayload forSlot(Weekday day, std::size_t slot, bool copy);
    [[nodiscard]] static std::optional<DragPayload> deserialize(std::string_view text);

    [[nodiscard]] std::string serialize() const;
    [[nodiscard]] bool operator==(const DragPayload&) const = default;
};
