#pragma once

#include <Windows.h>
#include <string>
#include <vector>

// https://stackoverflow.com/a/57551892/11547162
inline void OutputFormattedString(const char* format, ...)
{
	// initialize use of the variable argument array
	va_list vaArgs;
	va_start(vaArgs, format);

	// reliably acquire the size
	// from a copy of the variable argument array
	// and a functionally reliable call to mock the formatting
	va_list vaArgsCopy;
	va_copy(vaArgsCopy, vaArgs);
	const int iLen = std::vsnprintf(NULL, 0, format, vaArgsCopy);
	va_end(vaArgsCopy);

	// return a formatted string without risking memory mismanagement
	// and without assuming any compiler or platform specific behavior
	std::vector<char> zc(iLen + 1);
	std::vsnprintf(zc.data(), zc.size(), format, vaArgs);
	va_end(vaArgs);
	std::string strText(zc.data(), iLen);

	OutputDebugStringA(strText.c_str());
}

bool StringStartsWith(const wchar_t* string, wchar_t* cmp);