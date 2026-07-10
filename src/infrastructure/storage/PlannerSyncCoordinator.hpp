#pragma once

#include "../../core/model/Snapshot.hpp"
#include "stapik/cloud/CloudStorageClient.hpp"

class PlannerSyncCoordinator
{
public:
    // Whole-document last-write-wins.
    [[nodiscard]] static Snapshot resolveOnConnect(const Snapshot& local, CloudStorageClient& cloudClient);
};