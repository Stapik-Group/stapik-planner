#pragma once

#include "stapik/locale/LocaleManager.hpp"

#include <string>

inline std::string planDisplayName(const std::string& name)
{
    return name.empty() ? LocaleManager::instance().translate("plan.default") : name;
}
