#pragma once



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
