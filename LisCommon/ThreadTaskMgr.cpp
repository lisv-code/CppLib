/****** Thread task manager implementation. (c) 2024-2025 LISV ******/
#include "ThreadTaskMgr.h"
#include <future>
#include <utility>

using namespace LisThread;
namespace ThreadTaskMgr_Imp
{
	const int ThreadWaitStopFinalMs = 320;
	const int ThreadWaitStopRestartMs = 660;
	const int ThreadWaitStopRequestMs = 1120;
	const int ThreadWaitTimeChunkMs = 60;
	const int ThreadWaitStopServiceMs = ThreadWaitStopFinalMs;
	const int ThreadServiceIdleMs = 600;
}
using namespace ThreadTaskMgr_Imp;

ThreadTaskMgr::ThreadTaskMgr(bool auto_cleanup)
{
	isAutoCleanup = auto_cleanup;
	serviceStopFlag = !isAutoCleanup;
	if (serviceStopFlag) {
		serviceThread = nullptr;
	} else {
		serviceThread = new std::thread(ServiceMainProc, this);
	}
}

ThreadTaskMgr::~ThreadTaskMgr()
{
	if (serviceThread) {
		serviceStopFlag = true;
		serviceThread->join();
		delete serviceThread;
	}
	std::lock_guard<std::mutex> sync_lock(task_list_sync);
	for (auto& item : tasks) {
		StopProc(*item.second, ThreadWaitStopFinalMs);
		delete item.second;
	}
}

ThreadTaskMgr::ThreadTask* ThreadTaskMgr::GetTask(const TaskId& task_id, bool auto_create)
{
	ThreadTask* result = nullptr;
	std::lock_guard<std::mutex> sync_lock(task_list_sync);
	const auto& it = tasks.find(task_id);
	if (it != tasks.end())
		result = (*it).second;
	else if (auto_create) {
		auto task_data = new ThreadTask{};
		auto item = tasks.insert(std::make_pair(task_id, task_data));
		result = (*item.first).second;
	}
	return result;
}

bool ThreadTaskMgr::StartProc(ThreadTask& task_item,
	TaskProc task_proc, TaskWorkData work_data, TaskFinCallback fin_callback)
{
	if (task_item.ProcThread) {
		if (task_item.IsProcFinished()) // The task was executed and already finished, do some cleanup
			StopProc(task_item, ThreadWaitStopRestartMs);
		else return false; // The task processing is still pending
	}
	if (task_proc) { // Start new task processing
		task_item.ProcCtrl.StopFlag = false;
		task_item.ProcCtrl.StopFunc = nullptr;
		task_item.ProcFinish = TimeValue_Empty;
		task_item.ProcStart = std::chrono::system_clock::now();
		task_item.ProcThread = new std::thread([&task_item, task_proc, work_data, fin_callback]() {
			task_item.ProcResult = task_proc(&task_item.ProcCtrl, work_data);
			task_item.ProcFinish = std::chrono::system_clock::now();
			if (fin_callback) fin_callback(task_item.ProcResult); // Callback after the task normally finished, not killed
		});
	}
	return nullptr != task_item.ProcThread;
}

bool ThreadTaskMgr::StartTask(const TaskId& task_id, TaskProc task_proc, TaskWorkData work_data,
	TaskFinCallback fin_callback)
{
	auto task = GetTask(task_id, true);
	return StartProc(*task, task_proc, work_data, fin_callback);
}

bool ThreadTaskMgr::WaitTask(const TaskId& task_id, int wait_time_ms)
{
	auto task = GetTask(task_id, false);
	if (!task)
		return false;

	return WaitProc(*task, wait_time_ms) >= 0;
}

bool ThreadTaskMgr::StopTask(const TaskId& task_id)
{
	auto task = GetTask(task_id, false);
	if (!task)
		return false;

	return StopProc(*task, ThreadWaitStopRequestMs);
}

TaskProcStatus ThreadTaskMgr::GetTaskStatus(const TaskId& task_id)
{
	TaskProcStatus result = tpsNone;
	auto task = GetTask(task_id, false);
	if (task) {
		bool is_thread_active = task->ProcThread && task->ProcThread->joinable();
		result = (is_thread_active && !task->IsProcFinished()) ? tpsProcessing : tpsFinished;
	}
	return result;
}

TimeDataType ThreadTaskMgr::GetTaskTime(const TaskId & task_id, TimeValueType type)
{
	auto task = GetTask(task_id, false);
	if (task) {
		switch (type) {
		case TimeValueType::tvtStart: return task->ProcStart;
		case TimeValueType::tvtFinish: return task->ProcFinish;
		}
	}
	return TimeValue_Empty;
}

bool ThreadTaskMgr::GetTaskResult(const TaskId& task_id, TaskProcResult& result)
{
	auto task = GetTask(task_id, false);
	if (!task)
		return false;

	result = task->ProcResult;
	return true;
}

int ThreadTaskMgr::WaitProc(ThreadTask& task_item, int wait_time_ms)
{
	std::thread* proc_thread = task_item.ProcThread;
	if (proc_thread->joinable()) {
		auto future = std::async(std::launch::async, &std::thread::join, proc_thread);
		std::future_status wait_status;
		while ((std::future_status::timeout
			== (wait_status = future.wait_for(std::chrono::milliseconds(ThreadWaitTimeChunkMs))))
			&& (wait_time_ms > 0))
		{
			wait_time_ms -= ThreadWaitTimeChunkMs;
		}
		return std::future_status::timeout != wait_status ? 1 : -1;
	}
	return 0;
}

bool ThreadTaskMgr::StopProc(ThreadTask& task_item, int wait_time_ms)
{
	std::thread* proc_thread = task_item.ProcThread;
	if (proc_thread) {
		if (task_item.ProcCtrl.StopFunc) task_item.ProcCtrl.StopFunc();
		task_item.ProcCtrl.StopFlag = true;
		if (WaitProc(task_item, wait_time_ms) < 0) { ; } // Probably something is wrong if thread has timed out
		delete proc_thread; // TODO: ? blocked thread may hang here, handle the case
		task_item.ProcThread = nullptr;
		task_item.ProcCtrl.StopFlag = false;
		task_item.ProcCtrl.StopFunc = nullptr;
		task_item.ProcFinish = std::chrono::system_clock::now();
		return true;
	} else
		return false;
}

void LisThread::ThreadTaskMgr::ServiceMainProc(ThreadTaskMgr* mgr)
{
	int idle_time = 0;
	while (!mgr->serviceStopFlag) {
		if (idle_time < ThreadServiceIdleMs) {
			std::this_thread::sleep_for(std::chrono::milliseconds(ThreadWaitTimeChunkMs));
			idle_time += ThreadWaitTimeChunkMs;
			continue;
		}
		idle_time = 0;

		if (mgr->isAutoCleanup) {
			std::lock_guard<std::mutex> sync_lock(mgr->task_list_sync);
			auto it = mgr->tasks.begin();
			while (it != mgr->tasks.end()) {
				auto task_item = it->second;
				if (task_item->IsProcFinished()) {
					mgr->StopProc(*task_item, ThreadWaitStopServiceMs);
					delete task_item;
					it = mgr->tasks.erase(it);
				} else {
					++it;
				}
			}
		}
	}
}
