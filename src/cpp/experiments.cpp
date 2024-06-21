// #include <Mlib/def.h>
// #include <algorithm> // For std::lower_bound
// #include <string_view>
// #include <utility>

// constexpr s32 S32MAX = 2147483647;

// constexpr s32 TITLE_BAR     = 1;
// constexpr s32 LINE_NUMBER   = 2;
// constexpr s32 GUIDE_STRIPE  = 3;
// constexpr s32 SCROLL_BAR    = 4;
// constexpr s32 SELECTED_TEXT = 5;
// constexpr s32 SPOTLIGHTED   = 6;
// constexpr s32 MINI_INFOBAR  = 7;
// constexpr s32 PROMPT_BAR    = 8;
// constexpr s32 STATUS_BAR    = 9;
// constexpr s32 ERROR_MESSAGE = 10;
// constexpr s32 KEY_COMBO     = 11;
// constexpr s32 FUNCTION_TAG  = 12;

// constexpr std::pair<const char *, s32> colorOptionsMap[] __attribute__((unused)) = {
//     {    "titlecolor",     TITLE_BAR},
//     {   "numbercolor",   LINE_NUMBER},
//     {   "stripecolor",  GUIDE_STRIPE},
//     { "scrollercolor",    SCROLL_BAR},
//     { "selectedcolor", SELECTED_TEXT},
//     {"spotlightcolor",   SPOTLIGHTED},
//     {     "minicolor",  MINI_INFOBAR},
//     {   "promptcolor",    PROMPT_BAR},
//     {   "statuscolor",    STATUS_BAR},
//     {    "errorcolor", ERROR_MESSAGE},
//     {      "keycolor",     KEY_COMBO},
//     { "functioncolor",  FUNCTION_TAG}
// };

// constexpr s32 __attribute__((unused)) checkForColorOptions(const char *keyword)
// {
//     for (const auto &option : colorOptionsMap)
//     {
//         if (std::string_view(option.first) == keyword)
//         {
//             return option.second;
//         }
//     }
//     return S32MAX;
// }

// constexpr std::pair<std::string_view, s32> colorOptionsMapStringView[] __attribute__((unused)) = {
//     {    "errorcolor", ERROR_MESSAGE},
//     { "functioncolor",  FUNCTION_TAG},
//     {      "keycolor",     KEY_COMBO},
//     {     "minicolor",  MINI_INFOBAR},
//     {   "numbercolor",   LINE_NUMBER},
//     {   "promptcolor",    PROMPT_BAR},
//     { "scrollercolor",    SCROLL_BAR},
//     { "selectedcolor", SELECTED_TEXT},
//     {"spotlightcolor",   SPOTLIGHTED},
//     {   "statuscolor",    STATUS_BAR},
//     {   "stripecolor",  GUIDE_STRIPE},
//     {    "titlecolor",     TITLE_BAR}
// };

// constexpr s32 __attribute__((unused)) checkForColorOptions(std::string_view keyword)
// {
//     auto it = std::lower_bound(std::begin(colorOptionsMap), std::end(colorOptionsMap), keyword,
//                                [](const auto &lhs, const auto &rhs)
//                                {
//                                    return lhs.first < rhs;
//                                });

//     if (it != std::end(colorOptionsMap) && it->first == keyword)
//     {
//         return it->second;
//     }
//     return INT32_MAX;
// }

// #include <cstring>
// #include <iostream>

// class String
// {
// private:
//     char* data;
//     size_t length;

// public:
//     // Default constructor
//     String()
//     {
//         data = new char[1];
//         data[0] = '\0';
//         length = 0;
//     }

//     // Constructor with C-string
//     String(const char* str)
//     {
//         length = std::strlen(str);
//         data = new char[length + 1];
//         std::strcpy(data, str);
//     }

//     // Copy constructor
//     String(const String& other)
//     {
//         length = other.length;
//         data = new char[length + 1];
//         std::strcpy(data, other.data);
//     }

//     // Move constructor
//     String(String&& other) noexcept
//     {
//         data = other.data;
//         length = other.length;
//         other.data = nullptr;
//         other.length = 0;
//     }

//     // Destructor
//     ~String()
//     {
//         delete[] data;
//     }

//     // Copy assignment operator
//     String& operator=(const String& other)
//     {
//         if (this == &other)
//         {
//             return *this;
//         }

//         delete[] data;

//         length = other.length;
//         data = new char[length + 1];
//         std::strcpy(data, other.data);

//         return *this;
//     }

//     // Move assignment operator
//     String& operator=(String&& other) noexcept
//     {
//         if (this == &other)
//         {
//             return *this;
//         }

//         delete[] data;

//         data = other.data;
//         length = other.length;
//         other.data = nullptr;
//         other.length = 0;

//         return *this;
//     }

//     // Concatenation operator
//     String operator+(const String& other) const
//     {
//         String result;
//         result.length = length + other.length;
//         result.data = new char[result.length + 1];

//         std::strcpy(result.data, data);
//         std::strcat(result.data, other.data);

//         return result;
//     }

//     // Access operator
//     char& operator[](size_t index)
//     {
//         return data[index];
//     }

//     const char& operator[](size_t index) const
//     {
//         return data[index];
//     }

//     // Get length of the string
//     size_t size() const
//     {
//         return length;
//     }

//     // Get C-string
//     const char* c_str() const
//     {
//         return data;
//     }
// };

// template <u64 Size>
// struct constexprMap {
//     std::array<MapEntry, Size> entries;

//     constexpr constexprMap(std::array<MapEntry, Size> entries) : entries(entries) {}

//     constexpr u32 getValue(std::string_view key) const {
//         for (const auto& entry : entries) {
//             if (entry.key == key) {
//                 return entry.value;
//             }
//         }
//         return 0; // Default value if the key is not found
//     }
// };
// struct HashMapEntry
// {
//     constexpr HashMapEntry(std::string_view k, std::size_t v)
//         : key(k)
//         , value(v)
//     {}
//     std::string_view key;
//     std::size_t      value;
// };
