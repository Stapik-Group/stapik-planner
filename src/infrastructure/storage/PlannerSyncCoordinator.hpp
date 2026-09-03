#pragma once
#include "../../core/model/Snapshot.hpp"
#include "stapik/cloud/CloudStorageClient.hpp"

#include <optional>

class PlannerSyncCoordinator
{
public:
    [[nodiscard]] static Snapshot resolveOnConnect(const Snapshot& local, CloudStorageClient& cloudClient);
    [[nodiscard]] static Snapshot pushLocalChange(const Snapshot& local, CloudStorageClient& cloudClient);
private:
    [[nodiscard]] static Snapshot pushWithConflictResolution(
        const Snapshot& local,
        CloudStorageClient& cloudClient,
        std::optional<std::chrono::system_clock::time_point> baseline);

    [[nodiscard]] static Snapshot fromCloudDocument(const CloudDocument& document);
};