#pragma once

#include <chrono>

double get_time()
{
	auto time = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration<double>(time).count();
}
