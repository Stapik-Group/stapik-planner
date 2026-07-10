#pragma once

#include "stapik/cloud/CloudStorageConfig.hpp"

#include <filesystem>
#include <optional>

class CloudStorageConfigStorage
{
public:
    static void save(const CloudStorageConfig& config);
    static std::optional<CloudStorageConfig> load();
private:
    static std::filesystem::path configPath();
};