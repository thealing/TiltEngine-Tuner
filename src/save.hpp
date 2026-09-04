#pragma once

#include <fstream>
#include <filesystem>
#include <vector>
#include <iostream>

class InputFileStream
{
public:
	InputFileStream(const std::filesystem::path& path) : _file(path, std::ios::binary)
	{
	}

	operator bool() const
	{
		return (bool)_file;
	}

	template<typename T>
	friend InputFileStream& operator>>(InputFileStream& self, T& value)
	{
		self._file.read((char*)&value, sizeof(T));
		return self;
	}

	template<typename T>
	friend InputFileStream& operator>>(InputFileStream& self, std::vector<T>& vec)
	{
		size_t count = 0;
		self._file.read((char*)&count, sizeof(count));
		vec.resize(count);
		self._file.read((char*)vec.data(), count * sizeof(T));
		return self;
	}

private:
	std::ifstream _file;
};

class OutputFileStream
{
public:
	OutputFileStream(const std::filesystem::path& path) : _file(path, std::ios::binary)
	{
	}

	operator bool() const
	{
		return (bool)_file;
	}

	template<typename T>
	friend OutputFileStream& operator<<(OutputFileStream& self, const T& value)
	{
		self._file.write((char*)&value, sizeof(T));
		return self;
	}

	template<typename T>
	friend OutputFileStream& operator<<(OutputFileStream& self, const std::vector<T>& vec)
	{
		size_t count = vec.size();
		self._file.write((const char*)&count, sizeof(count));
		self._file.write((const char*)vec.data(), count * sizeof(T));
		return self;
	}

private:
	std::ofstream _file;
};
