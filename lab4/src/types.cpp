#include "types.hpp"

std::wstring SubStringsFindResult::serialize()
{
	std::wstring result;
	result += mFileName;
	result += L": substring count: ";
	result += std::to_wstring(mPostions.size());
	result += L", Substrings positions: { ";

	bool f = true;
	for (auto& e : mPostions)
		result += (f ? (f = false, L"") : L", ") + std::to_wstring(e);
	result += L"}\n";

	return result;
}

