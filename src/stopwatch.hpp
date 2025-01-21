#pragma once

#include "time.hpp"

#include <string>
#include <iostream>

class Stopwatch
{
public:
	Stopwatch(std::string name)
	{
		_name = name;
		_start_time = get_time();
		std::cout << _name << " started" << std::endl;
	}

	~Stopwatch()
	{
		double elapsed_time = get_time() - _start_time;
		std::cout << _name << " took " << elapsed_time << " seconds" << std::endl;
	}

private:
	std::string _name;
	double _start_time;
};
