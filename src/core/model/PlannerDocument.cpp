#include "PlannerDocument.hpp"

#include "stapik/sync/Timestamp.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    using SlotList = std::vector<std::optional<Activity>>;

    WeekPlan createEmptyWeekPlan(const int slotsCount)
    {
        using enum Weekday;
        return WeekPlan{
            DayPlan{ MONDAY, slotsCount },
            DayPlan{ TUESDAY, slotsCount },
            DayPlan{ WEDNESDAY, slotsCount },
            DayPlan{ THURSDAY, slotsCount },
            DayPlan{ FRIDAY, slotsCount },
            DayPlan{ SATURDAY, slotsCount },
            DayPlan{ SUNDAY, slotsCount }
        };
    }

    nlohmann::json activityToJson(const Activity& activity)
    {
        return {
            { "name", activity.name },
            { "difficulty", Activity::serializeDifficulty(activity.difficulty) }
        };
    }

    Activity activityFromJson(const nlohmann::json& json)
    {
        return Activity{
            .name = json.at("name").get<std::string>(),
            .difficulty = Activity::deserializeDifficulty(json.at("difficulty").get<std::string>())
        };
    }

    nlohmann::json activitiesToJson(const Activities& activities)
    {
        auto json = nlohmann::json::array();
        for (const auto& activity : activities)
            json.push_back(activityToJson(activity));

        return json;
    }

    Activities activitiesFromJson(const nlohmann::json& json)
    {
        Activities activities;
        if (!json.contains("activities"))
            return activities;

        for (const auto& activityJson : json.at("activities"))
            activities.push_back(activityFromJson(activityJson));

        return activities;
    }

    nlohmann::json settingsToJson(const Settings& settings)
    {
        return { { "slots", settings.slots } };
    }

    Settings settingsFromJson(const nlohmann::json& json)
    {
        Settings settings;
        if (json.contains("settings"))
        {
            const auto storedSlots = json.at("settings").at("slots").get<int>();
            settings.slots = std::clamp(storedSlots, Settings::MIN_SLOTS, Settings::MAX_SLOTS);
        }

        return settings;
    }

    nlohmann::json slotsToJson(const SlotList& slots)
    {
        auto json = nlohmann::json::array();
        for (const auto& slot : slots)
        {
            if (slot.has_value())
                json.push_back(activityToJson(slot.value()));
            else
                json.push_back(nullptr);
        }

        return json;
    }

    SlotList slotsFromJson(const nlohmann::json& json, const int slotsCount)
    {
        SlotList slots(static_cast<std::size_t>(slotsCount), std::nullopt);

        std::size_t slotIndex = 0;
        for (const auto& slotJson : json)
        {
            if (slotIndex >= slots.size())
                break;

            if (!slotJson.is_null())
                slots[slotIndex] = activityFromJson(slotJson);

            ++slotIndex;
        }

        return slots;
    }

    nlohmann::json weekPlanToJson(const WeekPlan& weekPlan)
    {
        auto json = nlohmann::json::array();
        for (const auto& day : weekPlan)
        {
            json.push_back({
                { "weekday", DayPlan::serializeWeekday(day.weekday) },
                { "slots", slotsToJson(day.slots) }
            });
        }

        return json;
    }

    WeekPlan weekPlanFromJson(const nlohmann::json& json, const int slotsCount)
    {
        auto weekPlan = createEmptyWeekPlan(slotsCount);
        if (!json.contains("weekPlan"))
            return weekPlan;

        for (const auto& dayJson : json.at("weekPlan"))
        {
            const auto weekday = DayPlan::deserializeWeekday(dayJson.at("weekday").get<std::string>());
            weekPlan[static_cast<std::size_t>(weekday)].slots = slotsFromJson(dayJson.at("slots"), slotsCount);
        }

        return weekPlan;
    }
}

PlannerDocument PlannerDocument::createDefault()
{
    return PlannerDocument{
        .settings = Settings{},
        .activities = Activities{},
        .weekPlan = createEmptyWeekPlan(Settings::DEFAULT_SLOTS),
        .updatedAt = TimePoint{},
        .cloudBaseline = std::nullopt
    };
}

PlannerDocument PlannerDocument::fromJson(const nlohmann::json& json)
{
    if (!json.is_object())
        throw std::invalid_argument("The planner document must be a JSON object");

    const auto settings = settingsFromJson(json);
    const auto updatedAt = json.contains("lastUpdate")
        ? stapik::sync::fromIso8601(json.at("lastUpdate").get<std::string>())
        : TimePoint{};

    return PlannerDocument{
        .settings = settings,
        .activities = activitiesFromJson(json),
        .weekPlan = weekPlanFromJson(json, settings.slots),
        .updatedAt = updatedAt,
        .cloudBaseline = std::nullopt
    };
}

nlohmann::json PlannerDocument::toJson() const
{
    nlohmann::json json = nlohmann::json::object();
    json["lastUpdate"] = stapik::sync::toIso8601(updatedAt);
    json["settings"] = settingsToJson(settings);
    json["activities"] = activitiesToJson(activities);
    json["weekPlan"] = weekPlanToJson(weekPlan);

    return json;
}

PlannerDocument::TimePoint PlannerDocument::lastUpdate() const
{
    return updatedAt;
}

std::optional<PlannerDocument::TimePoint> PlannerDocument::lastKnownCloudUpdate() const
{
    return cloudBaseline;
}

PlannerDocument PlannerDocument::withLastKnownCloudUpdate(const TimePoint cloudUpdate) const
{
    auto copy = *this;
    copy.cloudBaseline = cloudUpdate;
    return copy;
}
