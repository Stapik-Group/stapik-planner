#include "DragPayload.hpp"

#include <charconv>
#include <format>
#include <utility>

namespace
{
    constexpr std::string_view ACTIVITY_PREFIX = "activity:";
    constexpr std::string_view SLOT_PREFIX = "slot:";
    constexpr std::size_t LAST_WEEKDAY_INDEX = 6;

    std::optional<std::size_t> parseIndex(const std::string_view text)
    {
        std::size_t value = 0;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (error != std::errc{} || end != text.data() + text.size())
            return std::nullopt;

        return value;
    }
}

DragPayload DragPayload::forActivity(std::string activityId)
{
    return DragPayload{ .kind = Kind::Activity, .activityId = std::move(activityId) };
}

DragPayload DragPayload::forSlot(const Weekday day, const std::size_t slot, const bool copy)
{
    return DragPayload{ .kind = Kind::Slot, .day = day, .slot = slot, .copy = copy };
}

std::string DragPayload::serialize() const
{
    if (kind == Kind::Activity)
        return std::format("{}{}", ACTIVITY_PREFIX, activityId);

    return std::format("{}{}:{}:{}", SLOT_PREFIX, static_cast<std::size_t>(day), slot, copy ? 1 : 0);
}

std::optional<DragPayload> DragPayload::deserialize(const std::string_view text)
{
    if (text.starts_with(ACTIVITY_PREFIX))
    {
        const auto id = text.substr(ACTIVITY_PREFIX.size());
        if (id.empty())
            return std::nullopt;

        return forActivity(std::string(id));
    }

    if (!text.starts_with(SLOT_PREFIX))
        return std::nullopt;

    const auto rest = text.substr(SLOT_PREFIX.size());
    const auto firstSeparator = rest.find(':');
    const auto secondSeparator = firstSeparator == std::string_view::npos ? firstSeparator : rest.find(':', firstSeparator + 1);
    if (secondSeparator == std::string_view::npos)
        return std::nullopt;

    const auto day = parseIndex(rest.substr(0, firstSeparator));
    const auto slot = parseIndex(rest.substr(firstSeparator + 1, secondSeparator - firstSeparator - 1));
    const auto copy = parseIndex(rest.substr(secondSeparator + 1));
    if (!day || !slot || !copy || *day > LAST_WEEKDAY_INDEX || *copy > 1)
        return std::nullopt;

    return forSlot(static_cast<Weekday>(*day), *slot, *copy == 1);
}
