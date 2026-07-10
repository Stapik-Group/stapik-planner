#include "PlannerSyncCoordinator.hpp"
#include "Storage.hpp"

#include "stapik/cloud/CloudStorageException.hpp"

Snapshot PlannerSyncCoordinator::resolveOnConnect(const Snapshot& local, CloudStorageClient& cloudClient)
{
    try
    {
        const auto json = cloudClient.loadJson();
        if (json.empty())
        {
            cloudClient.saveJson(Storage::toJson(local));
            return local;
        }

        const auto cloud = Storage::fromJson(json);

        if (cloud.lastUpdate > local.lastUpdate)
        {
            Storage::save(cloud);
            return cloud;
        }

        if (local.lastUpdate > cloud.lastUpdate)
        {
            try { cloudClient.saveJson(Storage::toJson(local)); }
            catch (const CloudStorageException&) { /* will retry on next save */ }
        }

        return local;
    }
    catch (const CloudStorageException&)
    {
        return local;
    }
}