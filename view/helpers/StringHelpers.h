#pragma once

#include <algorithm>
#include <locale>
#include <cctype>

static bool ichar_equals(char a, char b)
{
    return std::tolower(static_cast<unsigned char>(a)) ==
        std::tolower(static_cast<unsigned char>(b));
}

static bool iequals(std::string_view lhs, std::string_view rhs)
{
    return std::ranges::equal(lhs, rhs, ichar_equals);
}

static std::string to_lower(std::string a)
{
    std::ranges::transform(a, a.begin(),
                           [](unsigned char c) { return std::tolower(c); });
    return a;
}

static bool contains(const std::string& a, const std::string& b)
{
    return a.find(b) != std::string::npos;
}
