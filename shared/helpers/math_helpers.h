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
 * \brief Computes the average rate of entries in the time queue per second (e.g.: FPS from frame deltas)
 * \param times A circular buffer of deltas
 * \return The average rate per second from the delta in the queue
 */
static float get_rate_per_second_from_deltas(const std::span<float>& times)
{
	size_t count = 0;
	float sum = 0.0f;
	for (const auto& time : times)
	{
		if (time > 0.0f)
		{
			sum += time;
			count++;
		}
	}
	return count > 0 ? 1000.0f / (sum / count) : 0.0f;
}

/**
 * \brief Formats a duration into a string of format HH:MM:SS
 * \param seconds The duration in seconds
 * \return The formatted duration
 */
static std::string format_duration(size_t seconds)
{
	char str[260] = {0};
	sprintf_s(str, "%02u:%02u:%02u", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
	return str;
}
