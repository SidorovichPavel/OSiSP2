#pragma once

#include <string>
#include <vector>
#include <iostream>

struct SubStringsFindResult
{
	std::wstring mFileName;
	std::vector<uint64_t> mPostions;

	std::wstring serialize();
};