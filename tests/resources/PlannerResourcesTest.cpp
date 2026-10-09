#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    namespace fs = std::filesystem;

    const fs::path PLANNER_SOURCE_DIR = STAPIK_PLANNER_SOURCE_DIR;
    const fs::path COMMON_RESOURCES_DIR = STAPIK_COMMON_RESOURCES_DIR;
    const std::vector<std::string> LANGUAGES = { "en", "pl", "de" };

    std::string readFile(const fs::path& path)
    {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::map<std::string, std::string> loadStrings(const fs::path& directory, const std::string& language)
    {
        const auto path = directory / "locales" / (language + ".json");
        std::ifstream file(path);
        EXPECT_TRUE(file.good()) << "missing " << path;
        return nlohmann::json::parse(file).get<std::map<std::string, std::string>>();
    }

    std::set<std::string> keysOf(const std::map<std::string, std::string>& strings)
    {
        std::set<std::string> keys;
        for (const auto& entry : strings)
            keys.insert(entry.first);

        return keys;
    }

    std::set<std::string> themeIdsIn(const fs::path& directory)
    {
        std::set<std::string> ids;
        for (const auto& entry : fs::directory_iterator(directory / "themes"))
        {
            if (entry.path().extension() == ".css")
                ids.insert(entry.path().stem().string());
        }

        return ids;
    }

    std::vector<fs::path> sourceFiles()
    {
        std::vector<fs::path> files;
        for (const auto& entry : fs::recursive_directory_iterator(PLANNER_SOURCE_DIR / "src"))
        {
            if (entry.path().extension() == ".cpp" || entry.path().extension() == ".hpp")
                files.push_back(entry.path());
        }

        return files;
    }
}

TEST(PlannerResourcesTest, EveryLanguageDefinesTheSameKeys)
{
    const auto expectedKeys = keysOf(loadStrings(PLANNER_SOURCE_DIR / "resources", "en"));

    for (const auto& language : LANGUAGES)
        EXPECT_EQ(keysOf(loadStrings(PLANNER_SOURCE_DIR / "resources", language)), expectedKeys) << "language " << language;
}

TEST(PlannerResourcesTest, NoTranslationIsEmpty)
{
    for (const auto& language : LANGUAGES)
    {
        for (const auto& [key, value] : loadStrings(PLANNER_SOURCE_DIR / "resources", language))
            EXPECT_FALSE(value.empty()) << key << " in " << language;
    }
}

// A key defined by the planner replaces the one of stapik-common in the whole application, including
// its dialogs, so it must only be defined when the planner needs a text of its own.
TEST(PlannerResourcesTest, PlannerKeysDoNotShadowKeysOfStapikCommon)
{
    for (const auto& language : LANGUAGES)
    {
        const auto commonKeys = keysOf(loadStrings(COMMON_RESOURCES_DIR, language));

        for (const auto& [key, value] : loadStrings(PLANNER_SOURCE_DIR / "resources", language))
            EXPECT_EQ(commonKeys.count(key), 0U) << key << " in " << language << " is already defined by stapik-common";
    }
}

// Literal keys of the planner namespaces found in the sources, e.g. translate("panel.activities.title")
// or an entry of a key table such as "day.mon".
TEST(PlannerResourcesTest, EveryKeyUsedInTheSourcesIsDefined)
{
    for (const auto& language : LANGUAGES)
    {
        auto definedKeys = keysOf(loadStrings(PLANNER_SOURCE_DIR / "resources", language));
        const auto commonKeys = keysOf(loadStrings(COMMON_RESOURCES_DIR, language));
        definedKeys.insert(commonKeys.begin(), commonKeys.end());

        std::set<std::string> prefixes;
        for (const auto& key : keysOf(loadStrings(PLANNER_SOURCE_DIR / "resources", language)))
            prefixes.insert(key.substr(0, key.find('.') + 1));

        const std::regex keyLiteral(R"re("([a-z]+(\.[A-Za-z]+)+)")re");
        for (const auto& file : sourceFiles())
        {
            const auto text = readFile(file);
            for (auto match = std::sregex_iterator(text.begin(), text.end(), keyLiteral); match != std::sregex_iterator(); ++match)
            {
                const auto key = (*match)[1].str();
                const auto prefix = key.substr(0, key.find('.') + 1);
                if (!prefixes.contains(prefix) && !key.starts_with("menu.") && !key.starts_with("cloud.") && !key.starts_with("dialog."))
                    continue;

                EXPECT_TRUE(definedKeys.contains(key)) << key << " used in " << file.filename() << " is not defined in " << language;
            }
        }
    }
}

// A theme of stapik-common without styles of the planner would show the planner widgets unstyled.
TEST(PlannerResourcesTest, EveryThemeOfStapikCommonHasStylesOfThePlanner)
{
    const auto commonThemes = themeIdsIn(COMMON_RESOURCES_DIR);
    const auto plannerThemes = themeIdsIn(PLANNER_SOURCE_DIR / "resources");

    for (const auto& themeId : commonThemes)
        EXPECT_TRUE(plannerThemes.contains(themeId)) << "no planner styles for the theme " << themeId;
}

TEST(PlannerResourcesTest, ThePlannerDoesNotStyleThemesThatDoNotExist)
{
    const auto commonThemes = themeIdsIn(COMMON_RESOURCES_DIR);

    for (const auto& themeId : themeIdsIn(PLANNER_SOURCE_DIR / "resources"))
        EXPECT_TRUE(commonThemes.contains(themeId)) << "planner styles for an unknown theme " << themeId;
}
