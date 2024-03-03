#pragma once
#include <algorithm>
#include <locale>
#include <vector>
#include <cctype>
#include <algorithm>
#include <string_view>

static bool ichar_equals(char a, char b)
{
	return std::tolower(static_cast<unsigned char>(a)) ==
		   std::tolower(static_cast<unsigned char>(b));
}

static bool iequals(std::string_view lhs, std::string_view rhs)
{
	return std::ranges::equal(lhs, rhs, ichar_equals);
}

inline static char* stristr(const char* str1, const char* str2)
{
	const char* p1 = str1;
	const char* p2 = str2;
	const char* r = *p2 == 0 ? str1 : 0;

	while (*p1 != 0 && *p2 != 0)
	{
		if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))
		{
			if (r == 0)
			{
				r = p1;
			}

			p2++;
		} else
		{
			p2 = str2;
			if (r != 0)
			{
				p1 = r + 1;
			}

			if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))
			{
				r = p1;
				p2++;
			} else
			{
				r = 0;
			}
		}

		p1++;
	}

	return *p2 == 0 ? const_cast<char*>(r) : 0;
}

inline static int is_string_alpha_only(const char* str)
{
	const size_t tempLen = strlen(str);
	for (size_t i = 0; i < tempLen; i++)
	{
		if (!isalpha(str[i]))
		{
			return 0;
		}
	}
	return 1;
}

inline static std::string to_lower(std::string a)
{
	std::ranges::transform(a, a.begin(),
	                       [](unsigned char c) { return std::tolower(c); });
	return a;
}

inline static bool contains(const std::string& a, const std::string& b)
{
	return a.find(b) != std::string::npos;
}

inline static std::wstring string_to_wstring(const std::string &str)
{
	const auto wstr = static_cast<wchar_t*>(calloc(str.length(), sizeof(wchar_t)+1));

	mbstowcs(wstr, str.c_str(), str.length());

	auto wstdstr = std::wstring(wstr);
	wstdstr.resize(str.length());

	free(wstr);
	return wstdstr;
}

static std::string wstring_to_string(const std::wstring& wstr) {
	std::string str(wstr.length(), 0);
	std::transform(wstr.begin(), wstr.end(), str.begin(), [] (wchar_t c) {
		return (char)c;
	});
	return str;
}

// https://stackoverflow.com/a/46931770/14472122
inline static std::vector<std::string> split_string(const std::string& s, const std::string& delimiter) {
	size_t pos_start = 0, pos_end;
	const size_t delim_len = delimiter.length();
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
		std::string token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back (token);
	}

	res.emplace_back (s.substr (pos_start));
	return res;
}

inline static std::vector<std::wstring> split_wstring(const std::wstring& s, const std::wstring& delimiter) {
	size_t pos_start = 0, pos_end;
	const size_t delim_len = delimiter.length();
	std::vector<std::wstring> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::wstring::npos) {
		std::wstring token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back (token);
	}

	res.emplace_back (s.substr (pos_start));
	return res;
}

inline static void strtrim(char* str, size_t len)
{
	for (int i = 0; i < len; ++i)
	{
		if (i == 0)
		{
			continue;
		}
		if (str[i - 1] == ' ' && str[i] == ' ')
		{
			str[i - 1] = '\0';
			return;
		}
	}
}
