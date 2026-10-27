#include "PlannerAppInfo.hpp"

#include "stapik/app/version.hpp"

#include <string>

stapik::app::AppInfo makePlannerAppInfo()
{
    return stapik::app::AppInfo{
        .applicationId = STAPIK_APP_APPLICATION_ID,
        .internalName = std::string{ stapik::app::build::NAME },
        .displayName = std::string{ stapik::app::build::DISPLAY_NAME },
        .version = std::string{ stapik::app::build::VERSION },
        .author = STAPIK_APP_AUTHOR,
        .repositoryUrl = STAPIK_APP_REPOSITORY_URL
    };
}
