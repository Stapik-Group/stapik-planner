#pragma once

#include "../../core/model/Snapshot.hpp"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <vector>

class StorageException : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class Storage
{
public:
    static void save(const Snapshot& snapshot);
    [[nodiscard]] static Snapshot load();
    [[nodiscard]] static nlohmann::json toJson(const Snapshot& snapshot);
    [[nodiscard]] static Snapshot fromJson(const nlohmann::json& json);
private:
    static std::filesystem::path storagePath();

    static std::string serializeTimestamp(std::chrono::system_clock::time_point tp);
    static std::chrono::system_clock::time_point deserializeTimestamp(const std::string& str);

    static nlohmann::json activitiesToJson(const Activities& activities);
    static Activities activitiesFromJson(const nlohmann::json& json);

    static nlohmann::json settingsToJson(const Settings& settings);
    static Settings settingsFromJson(const nlohmann::json& json);

    static nlohmann::json weekPlanToJson(const WeekPlan& weekPlan);
    static WeekPlan weekPlanFromJson(const nlohmann::json& json, int slotsCount);

    static nlohmann::json slotsToJson(const std::vector<std::optional<Activity>>& slots);
    static std::vector<std::optional<Activity>> slotsFromJson(const nlohmann::json& json, int slotsCount);

    static WeekPlan defaultWeekPlan(int slotsCount);
    static Snapshot defaultSnapshot();
};