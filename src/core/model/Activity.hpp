#pragma once

#include <string>

enum class Difficulty
{
    LIGHT,  // 10%
    NORMAL, // 20%
    MEDIUM, // 30%
    HARD    // 50%
};

struct Activity
{
    std::string name;
    Difficulty difficulty;

    static int difficultyLoad(const Difficulty difficulty)
    {
        switch (difficulty)
        {
            using enum Difficulty;
            case LIGHT: return 10;
            case NORMAL: return 20;
            case MEDIUM: return 30;
            case HARD: return 50;
        }
        return 0;
    }

    static std::string serializeDifficulty(const Difficulty difficulty)
    {
        switch (difficulty)
        {
            using enum Difficulty;
            case LIGHT: return "light";
            case NORMAL: return "normal";
            case MEDIUM: return "medium";
            case HARD: return "hard";
        }
        return "normal";
    }

    static Difficulty deserializeDifficulty(const std::string& str)
    {
        if (str == "light") return Difficulty::LIGHT;
        if (str == "medium") return Difficulty::MEDIUM;
        if (str == "hard") return Difficulty::HARD;
        return Difficulty::NORMAL;
    }
};