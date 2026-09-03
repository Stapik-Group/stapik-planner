#pragma once

#include <filesystem>

class CloudSchemaMigrationGuard
{
public:
    static void ensureCompatible();
private:
    static constexpr int CURRENT_SCHEMA_VERSION = 1;

    [[nodiscard]] static std::filesystem::path versionFilePath();
    [[nodiscard]] static int readStoredVersion();
    static void writeStoredVersion(int version);
};