#pragma once

#include "Activities.hpp"
#include "Settings.hpp"
#include "WeekPlan.hpp"

#include <chrono>

struct Snapshot
{
    Settings settings;
    Activities activities;
    WeekPlan weekPlan;
    std::chrono::system_clock::time_point lastUpdate;
};
