#include "winthreadpool.hpp"
#include "ConcurrencyList.hpp"

#include <fstream>
#include <atomic>

#include <Windows.h>

struct parameter_t
{
	cl::ConcurrencyList<std::wstring> FilesList;
	cl::ConcurrencyList<SubStringsFindResult> FindResultsList;
	std::string TargetSubStr;
};

size_t ThreadsCounter;
CONDITION_VARIABLE AllTasksComplited;
CRITICAL_SECTION ThreadsCounterLock;
TP_CALLBACK_ENVIRON CallbackEnviron;
PTP_WORK_CALLBACK WorkCallback;
PTP_POOL Pool;

void CALLBACK TaskFunc(PTP_CALLBACK_INSTANCE _Instance, PVOID _Parameter, PTP_WORK _Work)
{
	//std::wcout << GetThisThreadId() << std::endl;

	parameter_t& parameter = *static_cast<parameter_t*>(_Parameter);

	std::wstring filename = parameter.FilesList.pop();

	SubStringsFindResult ssfr;
	ssfr.mFileName = filename;

	std::ifstream fin(ssfr.mFileName);

	std::string text;
	std::copy(std::istreambuf_iterator<char>(fin), {}, std::back_insert_iterator<std::string>(text));

	for (auto pos = text.find(parameter.TargetSubStr);
		pos != std::string::npos;
		pos = text.find(parameter.TargetSubStr, pos + parameter.TargetSubStr.size()))
		ssfr.mPostions.push_back(pos);

	parameter.FindResultsList.push(ssfr);

	EnterCriticalSection(&ThreadsCounterLock);
	ThreadsCounter--;
	if (ThreadsCounter == 0)
		WakeConditionVariable(&AllTasksComplited);
	LeaveCriticalSection(&ThreadsCounterLock);
}

void win_threadpool(PTP_WORK _Work, size_t _Count)
{
	ThreadsCounter = _Count;

	for (auto i = 0; i < _Count; i++)
		SubmitThreadpoolWork(_Work);

	EnterCriticalSection(&ThreadsCounterLock);
	SleepConditionVariableCS(&AllTasksComplited, &ThreadsCounterLock, INFINITE);
	LeaveCriticalSection(&ThreadsCounterLock);
}

void init_winthreadpool()
{
	InitializeConditionVariable(&AllTasksComplited);
	InitializeCriticalSection(&ThreadsCounterLock);

	WorkCallback = TaskFunc;

	InitializeThreadpoolEnvironment(&CallbackEnviron);

	Pool = CreateThreadpool(0);
	if (!Pool)
		throw std::runtime_error("Threadpool create has been failed");

	SetThreadpoolThreadMaximum(Pool, 20);
	SetThreadpoolThreadMinimum(Pool, 6);
	SetThreadpoolCallbackPool(&CallbackEnviron, Pool);

}

void terminate_winthreadpool()
{
	CloseThreadpool(Pool);
}

cl::ConcurrencyList<SubStringsFindResult>
win_threadpoll_wrapper(cl::ConcurrencyList<std::wstring>& _Files, std::string _Substring)
{
	auto count = _Files.size();
	parameter_t parameter{ std::move(_Files), cl::ConcurrencyList<SubStringsFindResult>(), _Substring };

	PTP_WORK work = CreateThreadpoolWork(WorkCallback, &parameter, &CallbackEnviron);
	if (!work)
		throw std::runtime_error("Threadpool work create has been failed");

	win_threadpool(work, count);

	CloseThreadpoolWork(work);

	return std::move(parameter.FindResultsList);
}