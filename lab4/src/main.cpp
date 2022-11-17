#include <iostream>
#include <chrono>
#include <vector>
#include <fstream>
#include <filesystem>
#include <ranges>
#include <iterator>

#include <ThreadPool.hpp>

#include "ConcurrencyList.hpp"
#include "winthreadpool.hpp"

SubStringsFindResult find(std::filesystem::path _Path, const std::string& _SubStr)
{
	SubStringsFindResult ssfr;
	ssfr.mFileName = _Path.c_str();

	std::ifstream fin(ssfr.mFileName);

	std::string text;
	std::copy(std::istreambuf_iterator<char>(fin), {}, std::back_insert_iterator<std::string>(text));

	for (auto pos = text.find(_SubStr);
		pos != std::string::npos;
		pos = text.find(_SubStr, pos + _SubStr.size()))
		ssfr.mPostions.push_back(pos);

	return ssfr;
}

int wmain(int argc, wchar_t* args[])
{
	namespace stdfs = std::filesystem;

	constexpr size_t run_number = 2000;

	cl::ConcurrencyList<std::wstring> files;
	stdfs::path work_dir = "D:\\lab4\\";


	init_winthreadpool();

	std::ranges::for_each(stdfs::directory_iterator(work_dir),
		[&](const auto& dir) {
			files.push(stdfs::path(dir).c_str());
		});

	std::wcout << "Files count: " << files.size() << std::endl;
	auto win_threadpool_result = 
		win_threadpoll_wrapper(files, "Windows-приложений");
	for (auto e : win_threadpool_result)
		std::wcout << e.serialize();
	std::wcout << "Result count: " << win_threadpool_result.size() << std::endl;

	auto wintp_start_point = std::chrono::steady_clock::now();

	for (auto i = 0; i < run_number; i++)
	{

		std::ranges::for_each(stdfs::directory_iterator(work_dir),
			[&](const auto& dir) {
				files.push(stdfs::path(dir).c_str());
			});


		//std::wcout << "Files count: " << files.size() << std::endl;

		auto res = win_threadpoll_wrapper(files, "Windows-приложений");
	}

	auto wintp_end_point = std::chrono::steady_clock::now();

	terminate_winthreadpool();

	std::cout << "Winapi threadpool time result for " << run_number << " iterations: "
		<< std::chrono::duration_cast<std::chrono::nanoseconds>(wintp_end_point - wintp_start_point) << std::endl;

	ThreadPool threadpool(20);

	auto tp_start_point = std::chrono::steady_clock::now();

	for (auto i = 0; i < run_number; i++)
	{
		std::vector<std::future<SubStringsFindResult>> res;

		std::ranges::for_each(stdfs::directory_iterator(work_dir),
			[&threadpool, &res](const auto& dir) {
				res.push_back(
					threadpool.enqueue(find, stdfs::path(dir), "Windows-приложений")
				);
			});

		for (auto& f : res)
			f.get();
	}

	auto tp_end_point = std::chrono::steady_clock::now();

	std::cout << "My threadpool time result for " << run_number << " iterations: "
		<< std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end_point - tp_start_point) << std::endl;

	return 0;
}