#include "CloudStorageConfigStorage.hpp"
#include "stapik/cloud/CloudStorageException.hpp"

#include <nlohmann/json.hpp>
#include <fstream>

void CloudStorageConfigStorage::save(const CloudStorageConfig &config)
{
    const auto path = configPath();
    std::filesystem::create_directories(path.parent_path());

    const nlohmann::json json = {
        { "apiUrl", config.apiUrl },
        { "apiKey", config.apiKey }
    };

    std::ofstream file(path);
    file << json.dump(2);
}

std::optional<CloudStorageConfig> CloudStorageConfigStorage::load()
{
    const auto path = configPath();
    if (!std::filesystem::exists(path))
        return std::nullopt;

    std::ifstream file(path);
    if (!file.is_open())
        return std::nullopt;

    try
    {
        const auto json = nlohmann::json::parse(file);

        return CloudStorageConfig{
            json.at("apiUrl").get<std::string>(),
            json.at("apiKey").get<std::string>()
        };
    }
    catch (const nlohmann::json::exception&)
    {
        return std::nullopt;
    }
}

std::filesystem::path CloudStorageConfigStorage::configPath()
{
    const auto* home = std::getenv("HOME");
    if (home == nullptr)
        throw CloudStorageException("No HOME in PATH");

    return std::filesystem::path(home) / ".local" / "share" / "stapikplanner" / "config.json";
}