#pragma once
#include "../../core/model/PlannerDocument.hpp"
#include "stapik/cloud/CloudStorageClient.hpp"

#include <optional>

class PlannerSyncCoordinator
{
public:
    [[nodiscard]] static PlannerDocument resolveOnConnect(const PlannerDocument& local, CloudStorageClient& cloudClient);
    [[nodiscard]] static PlannerDocument pushLocalChange(const PlannerDocument& local, CloudStorageClient& cloudClient);
private:
    [[nodiscard]] static PlannerDocument pushWithConflictResolution(
        const PlannerDocument& local,
        CloudStorageClient& cloudClient,
        std::optional<std::chrono::system_clock::time_point> baseline);

    [[nodiscard]] static PlannerDocument fromCloudDocument(const CloudDocument& cloudDocument, const PlannerDocument& fallback);
};