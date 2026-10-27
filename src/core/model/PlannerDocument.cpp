#include "PlannerDocument.hpp"

#include "stapik/domain/IdGenerator.hpp"
#include "stapik/log/Log.hpp"
#include "stapik/sync/Timestamp.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
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
            { "id", activity.id },
            { "name", activity.name },
            { "difficulty", Activity::serializeDifficulty(activity.difficulty) }
        };
    }

    Activity activityFromJson(const nlohmann::json& json)
    {
        const auto difficultyText = json.at("difficulty").get<std::string>();
        const auto difficulty = Activity::deserializeDifficulty(difficultyText);
        if (!difficulty.has_value())
            throw std::invalid_argument(std::format("Unknown difficulty \"{}\"", difficultyText));

        return Activity{
            .id = json.value("id", std::string{}),
            .name = json.at("name").get<std::string>(),
            .difficulty = *difficulty
        };
    }

    nlohmann::json activitiesToJson(const Activities& activities)
    {
        auto json = nlohmann::json::array();
        for (const auto& activity : activities)
            json.push_back(activityToJson(activity));

        return json;
    }

    // Documents written before activities had ids, or damaged ones, may lack ids or repeat them.
    void assignMissingIds(Activities& activities)
    {
        std::unordered_set<std::string> usedIds;
        for (auto& activity : activities)
        {
            if (!activity.id.empty() && usedIds.insert(activity.id).second)
                continue;

            do
            {
                activity.id = stapik::domain::generateId();
            }
            while (!usedIds.insert(activity.id).second);
        }
    }

    Activities activitiesFromJson(const nlohmann::json& json)
    {
        Activities activities;
        if (json.contains("activities"))
        {
            for (const auto& activityJson : json.at("activities"))
                activities.push_back(activityFromJson(activityJson));
        }

        assignMissingIds(activities);
        return activities;
    }

    const Activity* findActivityById(const Activities& activities, const std::string& id)
    {
        for (const auto& activity : activities)
        {
            if (activity.id == id)
                return &activity;
        }

        return nullptr;
    }

    // Earlier versions identified a planned activity by its name only.
    const Activity* findActivityForSlotWithoutId(const Activities& activities, const Activity& slot)
    {
        const Activity* sameNameOnly = nullptr;
        for (const auto& activity : activities)
        {
            if (activity.name != slot.name)
                continue;

            if (activity.difficulty == slot.difficulty)
                return &activity;

            if (sameNameOnly == nullptr)
                sameNameOnly = &activity;
        }

        return sameNameOnly;
    }

    // The catalog is the authority: the slot takes the current name and difficulty of its activity,
    // and a slot whose activity is gone is dropped, which is what the window showed for it anyway.
    std::optional<Activity> resolveSlot(const Activity& storedSlot, const Activities& activities)
    {
        const auto* activity = storedSlot.id.empty()
            ? findActivityForSlotWithoutId(activities, storedSlot)
            : findActivityById(activities, storedSlot.id);

        if (activity == nullptr)
        {
            stapik::log::warning("The planned activity \"{}\" is not in the catalog, its slot is dropped", storedSlot.name);
            return std::nullopt;
        }

        return *activity;
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

    SlotList slotsFromJson(const nlohmann::json& json, const int slotsCount, const Activities& activities)
    {
        SlotList slots(static_cast<std::size_t>(slotsCount), std::nullopt);

        std::size_t slotIndex = 0;
        for (const auto& slotJson : json)
        {
            if (slotIndex >= slots.size())
                break;

            if (!slotJson.is_null())
                slots[slotIndex] = resolveSlot(activityFromJson(slotJson), activities);

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

    WeekPlan weekPlanFromJson(const nlohmann::json& json, const int slotsCount, const Activities& activities)
    {
        auto weekPlan = createEmptyWeekPlan(slotsCount);
        if (!json.contains("weekPlan"))
            return weekPlan;

        for (const auto& dayJson : json.at("weekPlan"))
        {
            const auto weekdayText = dayJson.at("weekday").get<std::string>();
            const auto weekday = DayPlan::deserializeWeekday(weekdayText);
            if (!weekday.has_value())
                throw std::invalid_argument(std::format("Unknown weekday \"{}\"", weekdayText));

            weekPlan[static_cast<std::size_t>(*weekday)].slots = slotsFromJson(dayJson.at("slots"), slotsCount, activities);
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

    auto activities = activitiesFromJson(json);
    auto weekPlan = weekPlanFromJson(json, settings.slots, activities);

    return PlannerDocument{
        .settings = settings,
        .activities = std::move(activities),
        .weekPlan = std::move(weekPlan),
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
