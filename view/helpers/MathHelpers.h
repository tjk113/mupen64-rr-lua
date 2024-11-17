#pragma once

#include <span>
#include <string>

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
 * \brief Computes the average rate of entries in the time queue per second (e.g.: FPS from frame deltas)
 * \param times A circular buffer of deltas
 * \return The average rate per second from the delta in the queue
 */
static double get_rate_per_second_from_deltas(const std::span<double>& times)
{
    size_t count = 0;
    double sum = 0.0;
    for (const auto& time : times)
    {
        if (time > 0.0)
        {
            sum += time;
            count++;
        }
    }
    return count > 0 ? 1000.0 / (sum / count) : 0.0;
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
