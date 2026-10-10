#pragma once

#include "../../core/model/PlannerDocument.hpp"

#include "stapik/document/DocumentFile.hpp"

#include <filesystem>
#include <string_view>

class PlannerStorage
{
public:
    static constexpr int SCHEMA_VERSION = 1;

    PlannerStorage();
    explicit PlannerStorage(std::filesystem::path filePath);

    [[nodiscard]] PlannerDocument load() const;
    [[nodiscard]] bool save(const PlannerDocument& document) const;
    [[nodiscard]] const std::filesystem::path& filePath() const;

    [[nodiscard]] static std::filesystem::path defaultFilePath();
private:
    static constexpr auto FILE_NAME = "planner.json";

    void setAsideUnreadableFile(std::string_view reason) const;

    stapik::document::DocumentFile<PlannerDocument> m_documentFile;
};
