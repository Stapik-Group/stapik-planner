#include "PlannerSyncCoordinator.hpp"
#include "Storage.hpp"
#include "stapik/cloud/CloudStorageException.hpp"

#include <glib.h>

Snapshot PlannerSyncCoordinator::fromCloudDocument(const CloudDocument& document)
{
    auto snapshot = Storage::fromJson(document.content);
    snapshot.lastKnownCloudUpdate = document.updatedAt;
    return snapshot;
}

Snapshot PlannerSyncCoordinator::pushWithConflictResolution(
    const Snapshot& local,
    CloudStorageClient& cloudClient,
    const std::optional<std::chrono::system_clock::time_point> baseline)
{
    CloudWriteResult result;

    try
    {
        result = cloudClient.saveDocument(Storage::toJson(local), baseline.value_or(std::chrono::system_clock::time_point{}));
    }
    catch (const CloudStorageException&)
    {
        g_debug("Cannot sync with cloud, will retry on next save.");
        return local;
    }

    if (!result.conflict)
    {
        auto snapshot = local;
        snapshot.lastKnownCloudUpdate = result.document.updatedAt;
        return snapshot;
    }

    // Server has a newer document than we knew about.
    if (result.document.updatedAt > local.lastUpdate)
        return fromCloudDocument(result.document);

    // We're still newer (rare race) — one retry against the server's current baseline.
    try
    {
        const auto [document, conflict] = cloudClient.saveDocument(Storage::toJson(local), result.document.updatedAt);

        if (!conflict)
        {
            auto snapshot = local;
            snapshot.lastKnownCloudUpdate = document.updatedAt;
            return snapshot;
        }

        // Lost the race twice — accept the server's version to avoid looping.
        return fromCloudDocument(document);
    }
    catch (const CloudStorageException&)
    {
        g_debug("Cannot sync with cloud, will retry on next save.");
        return local;
    }
}

Snapshot PlannerSyncCoordinator::resolveOnConnect(const Snapshot& local, CloudStorageClient& cloudClient)
{
    std::optional<CloudDocument> remote;

    try
    {
        remote = cloudClient.loadDocument();
    }
    catch (const CloudStorageException&)
    {
        g_debug("Cloud unreachable right now — keep working with local data.");
        return local;
    }

    if (!remote.has_value())
        return pushWithConflictResolution(local, cloudClient, std::nullopt);

    if (remote->updatedAt > local.lastUpdate)
        return fromCloudDocument(remote.value());

    return pushWithConflictResolution(local, cloudClient, remote->updatedAt);
}

Snapshot PlannerSyncCoordinator::pushLocalChange(const Snapshot& local, CloudStorageClient& cloudClient)
{
    return pushWithConflictResolution(local, cloudClient, local.lastKnownCloudUpdate);
}