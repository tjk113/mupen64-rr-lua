#pragma once

#include <filesystem>
#include <Windows.h>
#include <string>
#include <vector>
#include <queue>


/**
 * \brief Pushes an element to a circular buffer, limiting its size to the specified one by removing old elements
 * \tparam T The circular buffer's inner type
 * \param vec The circular buffer
 * \param value The value to push
 * \param max_size The size to limit the queue to
 */
template <typename T>
void circular_push(std::deque<T>& vec, const T value, const size_t max_size = 30)
{
	if (vec.size() > max_size)
	{
		vec.pop_front();
	}

	vec.push_back(value);
}

template <typename T>
T collection_average(std::deque<T>& vec)
{
	T sum = 0.0f;
	for (auto val : vec)
	{
		sum += val;
	}
	return sum / vec.size();
}
