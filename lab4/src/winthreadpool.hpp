#pragma once

#include "types.hpp"
#include "ConcurrencyList.hpp"

#include <string>
#include <vector>

void init_winthreadpool();
void terminate_winthreadpool();

cl::ConcurrencyList<SubStringsFindResult>
win_threadpoll_wrapper(cl::ConcurrencyList<std::wstring>& _Files, std::string _Substring);