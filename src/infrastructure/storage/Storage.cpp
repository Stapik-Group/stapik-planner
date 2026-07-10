#include "Storage.hpp"

#include <fstream>

#include "stapik/sync/SyncEnvelope.hpp"

void Storage::save(const Snapshot &snapshot)
{
    const auto path = storagePath();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    file << toJson(snapshot).dump(2);
}

Snapshot Storage::load()
{
    const auto path = storagePath();
    if (!std::filesystem::exists(path))
        return defaultSnapshot();

    std::ifstream file(path);
    if (!file.is_open())
        return defaultSnapshot();

    try
    {
        return fromJson(nlohmann::json::parse(file));
    }
    catch (const nlohmann::json::exception &)
    {
        return defaultSnapshot();
    }
}

nlohmann::json Storage::toJson(const Snapshot &snapshot)
{
    nlohmann::json payload;
    payload["settings"] = settingsToJson(snapshot.settings);
    payload["activities"] = activitiesToJson(snapshot.activities);
    payload["weekPlan"] = weekPlanToJson(snapshot.weekPlan);

    const stapik::sync::SyncEnvelope envelope{ snapshot.lastUpdate, payload };
    return envelope.toJson();
}

Snapshot Storage::fromJson(const nlohmann::json& json)
{
    try
    {
        const auto [lastUpdate, payload] = stapik::sync::SyncEnvelope::fromJson(json);
        const auto settings = settingsFromJson(payload);

        return Snapshot{
            settings,
            activitiesFromJson(payload),
            weekPlanFromJson(payload, settings.slots),
            lastUpdate
        };
    }
    catch (const nlohmann::json::exception&)
    {
        return defaultSnapshot();
    }
}

nlohmann::json Storage::activitiesToJson(const Activities &activities)
{
    nlohmann::json json = nlohmann::json::array();
    for (const auto &[name, difficulty] : activities)
    {
        json.push_back({
            { "name", name },
            { "difficulty", Activity::serializeDifficulty(difficulty) }
        });
    }

    return json;
}

Activities Storage::activitiesFromJson(const nlohmann::json& json)
{
    Activities activities;
    if (!json.contains("activities"))
        return activities;

    for (const auto& item : json.at("activities"))
    {
        activities.emplace_back(item.at("name").get<std::string>(),
            Activity::deserializeDifficulty(item.at("difficulty").get<std::string>()));
    }

    return activities;
}

nlohmann::json Storage::settingsToJson(const Settings &settings)
{
    return { { "slots", settings.slots } };
}

Settings Storage::settingsFromJson(const nlohmann::json& json)
{
    if (!json.contains("settings"))
        return {};

    Settings settings;
    settings.slots = json.at("settings").at("slots").get<int>();
    return settings;
}

nlohmann::json Storage::weekPlanToJson(const WeekPlan &weekPlan)
{
    nlohmann::json json = nlohmann::json::array();
    for (const auto& day : weekPlan)
    {
        json.push_back({
            { "weekday", DayPlan::serializeWeekday(day.weekday) },
            { "slots", slotsToJson(day.slots) }
        });
    }

    return json;
}

WeekPlan Storage::weekPlanFromJson(const nlohmann::json& json, const int slotsCount)
{
    WeekPlan weekPlan = defaultWeekPlan(slotsCount);
    if (!json.contains("weekPlan"))
        return weekPlan;

    for (const auto& dayJson : json.at("weekPlan"))
    {
        const auto weekday = DayPlan::deserializeWeekday(dayJson.at("weekday").get<std::string>());
        const auto index = static_cast<std::size_t>(weekday);

        weekPlan[index] = DayPlan{ weekday, slotsCount };
        weekPlan[index].slots = slotsFromJson(dayJson.at("slots"), slotsCount);
    }

    return weekPlan;
}

nlohmann::json Storage::slotsToJson(const std::vector<std::optional<Activity>> &slots)
{
    nlohmann::json json = nlohmann::json::array();
    for (const auto& slot : slots)
    {
        if (slot.has_value())
        {
            json.push_back({
                { "name", slot->name },
                { "difficulty", Activity::serializeDifficulty(slot->difficulty) }
            });
        }
        else
        {
            json.push_back(nullptr);
        }
    }

    return json;
}

std::vector<std::optional<Activity>> Storage::slotsFromJson(const nlohmann::json& json, const int slotsCount)
{
    std::vector<std::optional<Activity>> slots(static_cast<std::size_t>(slotsCount), std::nullopt);

    std::size_t i = 0;
    for (const auto& item : json)
    {
        if (i >= slots.size())
            break;

        if (!item.is_null())
        {
            slots[i] = Activity{
                item.at("name").get<std::string>(),
                Activity::deserializeDifficulty(item.at("difficulty").get<std::string>())
            };
        }

        ++i;
    }

    return slots;
}

WeekPlan Storage::defaultWeekPlan(const int slotsCount)
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

Snapshot Storage::defaultSnapshot()
{
    return Snapshot{
        Settings{},
        Activities{},
        defaultWeekPlan(Settings::DEFAULT_SLOTS),
        std::chrono::system_clock::time_point{}
    };
}

std::filesystem::path Storage::storagePath()
{
    const auto* home = std::getenv("HOME");
    if (home == nullptr)
        throw StorageException("No HOME in PATH");

    return std::filesystem::path(home) / ".local" / "share" / "stapikplanner" / "planner.json";
}