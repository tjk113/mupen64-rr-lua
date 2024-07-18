#pragma once

#include <algorithm>
#include <vector>
#include <algorithm>

static std::wstring string_to_wstring(const std::string& str)
{
	const auto wstr = static_cast<wchar_t*>(calloc(str.length(), sizeof(wchar_t) + 1));

	if (!wstr)
	{
		// Is it appropriate to fail silently...
		return L"";
	}

	mbstowcs(wstr, str.c_str(), str.length());

	auto wstdstr = std::wstring(wstr);
	wstdstr.resize(str.length());

	free(wstr);
	return wstdstr;
}

static std::string wstring_to_string(const std::wstring& wstr)
{
	std::string str(wstr.length(), 0);
	std::transform(wstr.begin(), wstr.end(), str.begin(), [](wchar_t c)
	{
		return (char)c;
	});
	return str;
}

// https://stackoverflow.com/a/46931770/14472122
static std::vector<std::string> split_string(const std::string& s, const std::string& delimiter)
{
	size_t pos_start = 0, pos_end;
	const size_t delim_len = delimiter.length();
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
	{
		std::string token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.emplace_back(s.substr(pos_start));
	return res;
}

static std::vector<std::wstring> split_wstring(const std::wstring& s, const std::wstring& delimiter)
{
	size_t pos_start = 0, pos_end;
	const size_t delim_len = delimiter.length();
	std::vector<std::wstring> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::wstring::npos)
	{
		std::wstring token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.emplace_back(s.substr(pos_start));
	return res;
}

static void strtrim(char* str, size_t len)
{
	for (int i = 0; i < len; ++i)
	{
		if (i == 0)
		{
			continue;
		}
		if (str[i - 1] == ' ' && str[i] == ' ')
		{
			memset(str + i - 1, 0, len - i + 1);
			return;
		}
	}
}

static size_t str_nth_occurence(const std::string& str, const std::string& searched, size_t nth)
{
	if (searched.empty() || nth <= 0)
	{
		return std::string::npos;
	}

	size_t pos = 0;
	int count = 0;

	while (count < nth)
	{
		pos = str.find(searched, pos);
		if (pos == std::string::npos)
		{
			return std::string::npos;
		}
		count++;
		if (count < nth)
		{
			pos += searched.size();
		}
	}

	return pos;
}
