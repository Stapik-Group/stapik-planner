#pragma once

#include "../../core/model/PlannerDocument.hpp"

#include "stapik/document/DocumentFile.hpp"
#include "stapik/storage/FileError.hpp"
#include "stapik/time/Clock.hpp"

#include <chrono>
#include <filesystem>
#include <string_view>
#include <vector>

class PlannerStorage
{
public:
    static constexpr int SCHEMA_VERSION = 1;
    static constexpr int BACKUP_COUNT = 5;
    static constexpr std::chrono::minutes BACKUP_INTERVAL{ 5 };

    PlannerStorage();
    explicit PlannerStorage(std::filesystem::path filePath, const stapik::time::IClock& clock = stapik::time::systemClock());

    [[nodiscard]] PlannerDocument load() const;
    [[nodiscard]] stapik::storage::FileResult save(const PlannerDocument& document) const;
    [[nodiscard]] std::vector<stapik::document::BackupInfo> listBackups() const;
    [[nodiscard]] stapik::document::LoadResult<PlannerDocument> loadBackup(const stapik::document::BackupInfo& backup) const;
    [[nodiscard]] const std::filesystem::path& filePath() const;

    [[nodiscard]] static std::filesystem::path defaultFilePath();
private:
    static constexpr auto FILE_NAME = "planner.json";

    [[nodiscard]] bool isBackupDue() const;
    void setAsideUnreadableFile(std::string_view reason) const;

    stapik::document::DocumentFile<PlannerDocument> m_documentFile;
    const stapik::time::IClock& m_clock;
};
