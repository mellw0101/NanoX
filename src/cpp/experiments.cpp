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
