#pragma once

#include "Activities.hpp"
#include "Settings.hpp"
#include "WeekPlan.hpp"

#include <chrono>
#include <optional>

struct Snapshot
{
    Settings settings;
    Activities activities;
    WeekPlan weekPlan;
    std::chrono::system_clock::time_point lastUpdate;
    std::optional<std::chrono::system_clock::time_point> lastKnownCloudUpdate;
};
