/// \file constexpr_utils.h
#pragma once

#include "constexpr_def.h"

//
//  Function to retrive a syntax option from a string literal.
//
constexpr u32
retriveSyntaxOptionFromStr(std::string_view key)
{
    for (const auto &entry : syntaxOptionMap)
    {
        if (entry.key == key)
        {
            return entry.value;
        }
    }
    return 0;
}

//
//  Function to retrive a color option from a string literal.
//
constexpr u32
retriveConfigOptionFromStr(std::string_view key)
{
    for (const auto &entry : configOptionMap)
    {
        if (entry.key == key)
        {
            return entry.value;
        }
    }
    return 0;
}

//
//  Function to retrive the color option from a string literal.
//
constexpr u32
retriveColorOptionFromStr(std::string_view key)
{
    for (const auto &[entry_key, entry_value] : colorOptionsMap)
    {
        if (entry_key == key)
        {
            return entry_value;
        }
    }
    return s32_MAX;
}

// constexpr u32
// retriveColorOptionFromStrHash(u64 hash)
// {
//     for (const auto &entry : colorOptionHashMap)
//     {
//         if (entry.hash == hash)
//         {
//             return entry.value;
//         }
//     }
//     return s32_MAX;
// }

//
//  Function to retrive flags from a string literal.
//
constexpr u32
retriveFlagFromStr(std::string_view key)
{
    for (const auto &entry : flagOptionsMap)
    {
        if (entry.key == key)
        {
            return entry.value;
        }
    }
    return 0;
}

constexpr u32
retriveCliOptionFromStr(std::string_view key)
{
    for (const auto &entry : cliOptionMap)
    {
        if (entry.key == key)
        {
            return entry.value;
        }
    }
    return 0;
}

//
//  Function to retrive the menu option from a string literal.
//
constexpr u32
nameToMenu(std::string_view key)
{
    for (const auto &entry : menuOptionMap)
    {
        if (entry.key == key)
        {
            return entry.value;
        }
    }
    return 0;
}

constexpr std::string_view
menuToName(u16 value)
{
    for (const auto &entry : menuOptionMap)
    {
        if (entry.value == value)
        {
            return entry.key;
        }
    }
    return "boooo";
}

//
//  Example usage of the hash_string function
//
/*
constexpr std::array<HashMapEntry, 6> syntaxOptionHashMap = {
    {{"color", "color"_hash},
     {"icolor", "icolor"_hash},
     {"comment", "comment"_hash},
     {"tabgives", "tabgives"_hash},
     {"linter", "linter"_hash},
     {"formatter", "formatter"_hash}}
};
constexpr std::size_t
retriveSyntaxOptionFromStrHash(std::string_view key)
{
    for (const auto &entry : syntaxOptionHashMap)
    {
        if (entry.key == key)
        {
            return entry.value;
        }
    }
    return u64_MAX;
}
*/
//
//  Example usage of the hash_string function
//
/*
int
main()
{
    constexpr auto color_hash  = get_value("color");
    constexpr auto linter_hash = get_value("linter");

    std::cout << "Hash for 'color': " << color_hash << std::endl;
    std::cout << "Hash for 'linter': " << linter_hash << std::endl;

    // Using the hash in a switch statement
    switch (hash_string("icolor"))
    {
        case "color"_hash :
            std::cout << "Color option selected." << std::endl;
            break;
        case "icolor"_hash :
            std::cout << "IColor option selected." << std::endl;
            break;
        case "comment"_hash :
            std::cout << "Comment option selected." << std::endl;
            break;
        default :
            std::cout << "Unknown option." << std::endl;
    }

    return 0;
}
*/

//
//  Compile-time string comparison function
//
constexpr bool
constexpr_strcmp(const s8 *str1, const s8 *str2)
{
    while (*str1 && (*str1 == *str2))
    {
        ++str1;
        ++str2;
    }
    return (*str1 == *str2);
}

constexpr s32
constexpr_strncmp(const s8 *s1, const s8 *s2, u64 n)
{
    for (std::size_t i = 0; i < n; ++i)
    {
        if (s1[i] == '\0' && s2[i] == '\0')
        {
            return 0;
        }
        if (s1[i] == '\0')
        {
            return -1;
        }
        if (s2[i] == '\0')
        {
            return 1;
        }
        if (s1[i] < s2[i])
        {
            return -1;
        }
        if (s1[i] > s2[i])
        {
            return 1;
        }
    }
    return 0;
}

template <u64 N>
constexpr void
constexpr_strncpy(char (&dest)[N], const s8 *src, u64 count)
{
    u64 i = 0;
    for (; i < count && src[i] != '\0'; ++i)
    {
        dest[i] = src[i];
    }
    for (; i < N; ++i)
    {
        dest[i] = '\0';
    }
}

template <std::size_t N>
struct ConstexprString
{
    s8 data[N + 1] = {};

    constexpr ConstexprString(const s8 *str)
    {
        constexpr_strncpy(data, str, N);
    }

    constexpr const s8 *
    c_str() const
    {
        return data;
    }
};
