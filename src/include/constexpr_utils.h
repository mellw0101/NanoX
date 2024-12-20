/** @file constexpr_utils.h */
#pragma once

#include "constexpr_def.h"

/* Function to retrive a syntax option from a string literal. */
constexpr int retriveSyntaxOptionFromStr(string_view str) {
  for (const auto &[key, val] : syntaxOptionMap) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}

/* Function to retrive a color option from a string literal. */
constexpr int retriveConfigOptionFromStr(string_view str) {
  for (const auto &[key, val] : configOptionMap) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}

/* Function to retrive the color option from a string literal. */
constexpr int retriveColorOptionFromStr(string_view str) {
  for (const auto &[key, val] : colorOptionMap) {
    if (key == str) {
      return val;
    }
  }
  return (Uint)-1;
}

/* Function to retrive flags from a string literal. */
constexpr Uint retriveFlagFromStr(string_view str) {
  for (const auto &[key, value] : flagOptionsMap) {
    if (key == str) {
      return value;
    }
  }
  return 0;
}

constexpr Uint retriveCliOptionFromStr(string_view str) {
  for (const auto &[key, val] : cliOptionMap) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}

constexpr Uint retriveToggleOptionFromStr(string_view str) {
  for (const auto &[key, value] : toggleOptionMap) {
    if (key == str) {
      return value;
    }
  }
  return 0;
}

/* Function to retrive the menu option from a string literal. */
constexpr Uint nameToMenu(string_view str) {
  for (const auto &[key, val] : menu_name_map) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}

constexpr string_view menu_to_name(const Ushort value) {
  for (const auto &[key, val] : menu_name_map) {
    if (val == value) {
      return key;
    }
  }
  return "boooo";
}

constexpr Uint retrieve_c_syntax_type(string_view str) {
  for (const auto &[key, val] : c_syntax_map) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}

constexpr Uint retrieve_preprossesor_type(std::string_view str) {
  for (const auto &[key, val] : c_preprossesor_map) {
    if (key == str) {
      return val;
    }
  }
  return 0;
}
