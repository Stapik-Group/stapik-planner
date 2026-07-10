#pragma once

struct Settings
{
    static constexpr int DEFAULT_SLOTS = 4;
    static constexpr int MIN_SLOTS = 1;
    static constexpr int MAX_SLOTS = 8;

    int slots = DEFAULT_SLOTS;
};