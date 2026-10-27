#include "PlannerStorage.hpp"

#include "stapik/app/AppContext.hpp"
#include "stapik/log/Log.hpp"
#include "stapik/storage/AppPaths.hpp"
#include "stapik/storage/PathText.hpp"

#include <chrono>
#include <format>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace
{
    // Schema 0 is the file written before the document was versioned: the sync envelope itself,
    // {"lastUpdate": ..., "payload": {settings, activities, weekPlan}, "lastKnownCloudUpdate": ...}.
    // The envelope is unwrapped into the document layout, which keeps "lastUpdate" next to the content.
    // The old cloud baseline is dropped: it is refreshed by the first synchronization anyway.
    void migrateLegacyEnvelope(nlohmann::json& document)
    {
        if (!document.is_object())
            throw std::invalid_argument("The legacy planner file must be a JSON object");

        nlohmann::json migrated = nlohmann::json::object();
        if (document.contains("payload"))
        {
            if (!document.at("payload").is_object())
                throw std::invalid_argument("The payload of the legacy planner file must be a JSON object");

            migrated = document.at("payload");
        }

        if (document.contains("lastUpdate"))
            migrated["lastUpdate"] = document.at("lastUpdate");

        document = std::move(migrated);
    }

    stapik::document::SchemaMigrator createMigrator()
    {
        stapik::document::SchemaMigrator migrator(PlannerStorage::SCHEMA_VERSION);
        migrator.addStep(0, migrateLegacyEnvelope);
        return migrator;
    }
}

PlannerStorage::PlannerStorage() :
    PlannerStorage(defaultFilePath())
{}

PlannerStorage::PlannerStorage(std::filesystem::path filePath, const stapik::time::IClock& clock) :
    m_documentFile(std::move(filePath), createMigrator(), clock),
    m_clock(clock)
{
    m_documentFile.setBackupCount(BACKUP_COUNT);
}

PlannerDocument PlannerStorage::load() const
{
    auto result = m_documentFile.load();

    switch (result.status)
    {
        using enum stapik::document::LoadStatus;
        case Loaded:
            if (result.migrated)
                stapik::log::info("Planner file migrated in memory from schema {} to {}", result.fileVersion, SCHEMA_VERSION);

            return std::move(result.document.value());
        case Missing:
            return PlannerDocument::createDefault();
        case Corrupted:
            stapik::log::warning("The planner file is unreadable, it was moved aside and an empty planner is used");
            return PlannerDocument::createDefault();
        case NewerVersion:
            setAsideUnreadableFile("newer-version");
            return PlannerDocument::createDefault();
        case MigrationFailed:
            setAsideUnreadableFile("migration-failed");
            return PlannerDocument::createDefault();
    }

    return PlannerDocument::createDefault();
}

stapik::storage::FileResult PlannerStorage::save(const PlannerDocument& document) const
{
    if (isBackupDue())
        return m_documentFile.save(document);

    return m_documentFile.exportTo(document, m_documentFile.filePath());
}

bool PlannerStorage::isBackupDue() const
{
    const auto backups = m_documentFile.listBackups();
    return backups.empty() || m_clock.now() - backups.front().savedAt >= BACKUP_INTERVAL;
}

std::vector<stapik::document::BackupInfo> PlannerStorage::listBackups() const
{
    return m_documentFile.listBackups();
}

stapik::document::LoadResult<PlannerDocument> PlannerStorage::loadBackup(const stapik::document::BackupInfo& backup) const
{
    return m_documentFile.loadBackup(backup);
}

const std::filesystem::path& PlannerStorage::filePath() const
{
    return m_documentFile.filePath();
}

std::filesystem::path PlannerStorage::defaultFilePath()
{
    const auto& applicationName = stapik::app::AppContext::instance().info().internalName;
    return AppPaths::userDataDir(applicationName) / FILE_NAME;
}

// A file written by a newer version, or one that cannot be migrated, is not corrupted: it must
// survive the next save, which would otherwise overwrite it with an empty planner.
void PlannerStorage::setAsideUnreadableFile(const std::string_view reason) const
{
    const auto timestamp = std::format("{:%Y%m%d-%H%M%S}", std::chrono::floor<std::chrono::seconds>(m_clock.now()));

    std::error_code errorCode;
    auto target = filePath();
    target += std::format(".{}-{}", reason, timestamp);

    int suffix = 2;
    while (std::filesystem::exists(target, errorCode))
    {
        target = filePath();
        target += std::format(".{}-{}-{}", reason, timestamp, suffix++);
    }

    std::filesystem::rename(filePath(), target, errorCode);
    if (errorCode)
    {
        stapik::log::warning("Cannot move the unreadable planner file {} aside: {}", stapik::storage::pathText(filePath()), errorCode.message());
        return;
    }

    stapik::log::warning("The planner file cannot be read by this version ({}), it was moved to {}", reason, stapik::storage::pathText(target));
}
