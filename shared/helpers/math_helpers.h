#pragma once
#include <chrono>
#include <deque>


/**
 * \brief Limits a value to a specific range
 * \param value The value to limit
 * \param min The lower bound
 * \param max The upper bound
 * \return The value, limited to the specified range
 */
template <typename T>
static T clamp(const T value, const T min, const T max)
{
	if (value > max)
	{
		return max;
	}
	if (value < min)
	{
		return min;
	}
	return value;
}


/**
 * \brief Computes the average rate of entries in the time queue per second (e.g.: FPS from frame time points)
 * \param times A queue of time points
 * \return The average rate of entries in the time queue per second
 */
static float get_rate_per_second_from_times(std::deque<std::chrono::high_resolution_clock::time_point> times)
{
	if (times.empty())
	{
		return 0.0;
	}

	long long fps = 0;
	for (int i = 1; i < times.size(); ++i)
	{
		fps += (times[i] - times[i - 1]).count();
	}
	return 1000.0f / (float)((fps / times.size()) / 1'000'000.0f);
}

/**
 * \brief Formats a duration into a string
 * \param seconds The duration in seconds
 * \return The formatted duration
 */
static std::string format_duration(size_t seconds)
{
	char tempbuf[260]{};
	double minutes = seconds / 60.0;
	if ((bool)seconds)
		seconds = fmod(seconds, 60.0);
	double hours = minutes / 60.0;
	if ((bool)minutes)
		minutes = fmod(minutes, 60.0);

	if (hours >= 1.0)
		sprintf(tempbuf, "%d hours and %.1f minutes", (unsigned int)hours,
		        (float)minutes);
	else if (minutes >= 1.0)
		sprintf(tempbuf, "%d minutes and %.0f seconds",
		        (unsigned int)minutes,
		        (float)seconds);
	else if (seconds > 0)
		sprintf(tempbuf, "%.1f seconds", (float)seconds);
	else
		strcpy(tempbuf, "0 seconds");
	return std::string(tempbuf);
}
