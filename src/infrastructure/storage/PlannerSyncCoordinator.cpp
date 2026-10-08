#include "PlannerSyncCoordinator.hpp"

#include "stapik/cloud/CloudStorageException.hpp"
#include "stapik/log/Log.hpp"
#include "stapik/sync/SyncEnvelope.hpp"

#include <glib.h>

#include <exception>

namespace
{
    nlohmann::json toCloudContent(const PlannerDocument& document)
    {
        return stapik::sync::SyncEnvelope{ .lastUpdate = document.lastUpdate(), .payload = document.toJson() }.toJson();
    }
}

PlannerDocument PlannerSyncCoordinator::fromCloudDocument(const CloudDocument& cloudDocument, const PlannerDocument& fallback)
{
    try
    {
        const auto envelope = stapik::sync::SyncEnvelope::fromJson(cloudDocument.content);
        return PlannerDocument::fromJson(envelope.payload).withLastKnownCloudUpdate(cloudDocument.updatedAt);
    }
    catch (const std::exception& exception)
    {
        stapik::log::warning("Cannot read the cloud document, keeping the local one: {}", exception.what());
        return fallback;
    }
}

PlannerDocument PlannerSyncCoordinator::pushWithConflictResolution(
    const PlannerDocument& local,
    CloudStorageClient& cloudClient,
    const std::optional<std::chrono::system_clock::time_point> baseline)
{
    CloudWriteResult result;

    try
    {
        result = cloudClient.saveDocument(toCloudContent(local), baseline.value_or(std::chrono::system_clock::time_point{}));
    }
    catch (const CloudStorageException&)
    {
        g_debug("Cannot sync with cloud, will retry on next save.");
        return local;
    }

    if (!result.conflict)
        return local.withLastKnownCloudUpdate(result.document.updatedAt);

    // Server has a newer document than we knew about.
    if (result.document.updatedAt > local.lastUpdate())
        return fromCloudDocument(result.document, local);

    // We're still newer (rare race) — one retry against the server's current baseline.
    try
    {
        const auto [document, conflict] = cloudClient.saveDocument(toCloudContent(local), result.document.updatedAt);

        if (!conflict)
            return local.withLastKnownCloudUpdate(document.updatedAt);

        // Lost the race twice — accept the server's version to avoid looping.
        return fromCloudDocument(document, local);
    }
    catch (const CloudStorageException&)
    {
        g_debug("Cannot sync with cloud, will retry on next save.");
        return local;
    }
}

PlannerDocument PlannerSyncCoordinator::resolveOnConnect(const PlannerDocument& local, CloudStorageClient& cloudClient)
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

    if (remote->updatedAt > local.lastUpdate())
        return fromCloudDocument(remote.value(), local);

    return pushWithConflictResolution(local, cloudClient, remote->updatedAt);
}

PlannerDocument PlannerSyncCoordinator::pushLocalChange(const PlannerDocument& local, CloudStorageClient& cloudClient)
{
    return pushWithConflictResolution(local, cloudClient, local.lastKnownCloudUpdate());
}
