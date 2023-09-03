#pragma once
#include <algorithm>

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
