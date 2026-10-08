#include "PlannerAppInfo.hpp"

stapik::app::AppInfo makePlannerAppInfo()
{
    return stapik::app::AppInfo{
        .applicationId = STAPIK_APP_APPLICATION_ID,
        .internalName = STAPIK_APP_NAME,
        .displayName = STAPIK_APP_DISPLAY_NAME,
        .version = STAPIK_APP_VERSION,
        .author = STAPIK_APP_AUTHOR,
        .repositoryUrl = STAPIK_APP_REPOSITORY_URL
    };
}
