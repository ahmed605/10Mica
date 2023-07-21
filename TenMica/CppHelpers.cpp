#include "CppHelpers.h"

bool StringStartsWith(const wchar_t* string, wchar_t* cmp)
{
    return std::wstring_view(string).starts_with(cmp);
}
