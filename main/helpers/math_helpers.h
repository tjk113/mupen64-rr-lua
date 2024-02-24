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
template<typename T>
T clamp(const T value, const T min, const T max)
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
float get_rate_per_second_from_times(std::deque<std::chrono::high_resolution_clock::time_point> times)
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
