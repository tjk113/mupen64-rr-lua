#pragma once
#include <algorithm>
#include <locale>
#include <codecvt>

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

	return *p2 == 0 ? (char*)r : 0;
}

inline static int is_string_alpha_only(const char* str)
{
	for (size_t i = 0; i < strlen(str); i++)
	{
		if (!isalpha(str[i]))
		{
			return 0;
		}
	}
	return 1;
}

// https://stackoverflow.com/a/4119881
inline static bool is_case_insensitive_equal(const std::string& a,
                                             const std::string& b)
{
	return std::equal(a.begin(), a.end(),
	                  b.begin(), b.end(),
	                  [](char a, char b)
	                  {
		                  return tolower(a) == tolower(b);
	                  });
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

inline static std::wstring string_to_wstring(const std::string str)
{
	const auto wstr = static_cast<wchar_t*>(calloc(str.length(), sizeof(wchar_t)));

	mbstowcs(wstr, str.c_str(), str.length());

	auto wstdstr = std::wstring(wstr);
	wstdstr.resize(str.length());

	free(wstr);
	return wstdstr;
}

inline static std::string wstring_to_string(const std::wstring& wstr) {
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

// https://stackoverflow.com/a/46931770/14472122
inline static std::vector<std::string> split_string(std::string s, const std::string& delimiter) {
	size_t pos_start = 0, pos_end;
	const size_t delim_len = delimiter.length();
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
		std::string token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back (token);
	}

	res.push_back (s.substr (pos_start));
	return res;
}

inline static std::vector<std::wstring> split_wstring(std::wstring s, const std::wstring& delimiter) {
	size_t pos_start = 0, pos_end;
	const size_t delim_len = delimiter.length();
	std::vector<std::wstring> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::wstring::npos) {
		std::wstring token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back (token);
	}

	res.push_back (s.substr (pos_start));
	return res;
}
