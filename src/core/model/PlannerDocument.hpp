#pragma once

#include "Activities.hpp"
#include "Settings.hpp"
#include "WeekPlan.hpp"

#include <nlohmann/json.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <vector>

struct SavedPlan
{
    std::string id;
    std::string name;
    WeekPlan weekPlan;

    [[nodiscard]] bool operator==(const SavedPlan&) const = default;
};

// The whole persisted and synchronized state of the planner.
//
// toJson() is both what is stored in the local file and what is uploaded to the cloud as the
// payload of the sync envelope, so it carries "lastUpdate" itself: a document that replaces the
// local one after a sync must know when it was last changed. The cloud baseline is local
// bookkeeping of the connection and is deliberately not serialized.
struct PlannerDocument
{
    using TimePoint = std::chrono::system_clock::time_point;

    Settings settings;
    Activities activities;
    WeekPlan weekPlan;
    TimePoint updatedAt;
    std::optional<TimePoint> cloudBaseline;

    static constexpr auto DEFAULT_PLAN_ID = "main";

    std::string activePlanId = DEFAULT_PLAN_ID;
    std::string activePlanName;
    std::vector<SavedPlan> savedPlans;

    [[nodiscard]] static WeekPlan emptyWeekPlan(int slotsCount);
    [[nodiscard]] static PlannerDocument createDefault();
    [[nodiscard]] static PlannerDocument fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json toJson() const;

    [[nodiscard]] TimePoint lastUpdate() const;
    [[nodiscard]] std::optional<TimePoint> lastKnownCloudUpdate() const;
    [[nodiscard]] PlannerDocument withLastKnownCloudUpdate(TimePoint cloudUpdate) const;
};
