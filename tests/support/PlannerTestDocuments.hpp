#pragma once

#include "core/model/PlannerDocument.hpp"

#include "stapik/cloud/ICloudStorage.hpp"
#include "stapik/sync/SyncEnvelope.hpp"

#include <chrono>

namespace planner::test
{
    using TimePoint = std::chrono::system_clock::time_point;

    inline TimePoint at(const int seconds)
    {
        using namespace std::chrono;
        return sys_days{ year{ 2026 } / January / 1 } + hours{ 12 } + std::chrono::seconds{ seconds };
    }

    // Two activities, a few filled slots, four slots per day (the default).
    inline PlannerDocument makeSampleDocument(const int updatedAtSeconds)
    {
        auto document = PlannerDocument::createDefault();
        document.updatedAt = at(updatedAtSeconds);
        document.activities = {
            Activity{ .id = "gym", .name = "Gym", .difficulty = Difficulty::HARD },
            Activity{ .id = "reading", .name = "Reading", .difficulty = Difficulty::LIGHT }
        };
        document.weekPlan[0].slots[0] = document.activities[0];
        document.weekPlan[0].slots[2] = document.activities[1];
        document.weekPlan[4].slots[1] = document.activities[1];
        return document;
    }

    // The document as the cloud stores it: the sync envelope around the payload, with the server's own time.
    inline CloudDocument cloudDocumentOf(const PlannerDocument& document, const int serverSeconds)
    {
        return CloudDocument{
            .content = stapik::sync::SyncEnvelope{ .lastUpdate = document.lastUpdate(), .payload = document.toJson() }.toJson(),
            .updatedAt = at(serverSeconds)
        };
    }
}
