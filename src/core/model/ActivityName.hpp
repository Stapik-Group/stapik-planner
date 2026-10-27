#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ActivityName
{
    inline constexpr std::size_t MAX_LENGTH = 60;

    [[nodiscard]] std::string trimmed(std::string_view name);
    [[nodiscard]] bool isTaken(std::string_view name, const std::vector<std::string>& otherNames);
}
