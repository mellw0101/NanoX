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
constexpr s32
retriveColorOptionFromStr(std::string_view key)
{
    for (const auto &[entry_key, entry_value] : colorOptionMap)
    {
        if (entry_key == key)
        {
            return entry_value;
        }
    }
    return s32_MAX;
}

//
//  Function to retrive flags from a string literal.
//
constexpr u32
retriveFlagFromStr(std::string_view str)
{
    for (const auto &[key, value] : flagOptionsMap)
    {
        if (key == str)
        {
            return value;
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
