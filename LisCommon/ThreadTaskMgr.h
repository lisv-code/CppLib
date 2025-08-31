/****** Thread task manager declaration. (c) 2024-2025 LISV ******/
#pragma once
#ifndef _LIS_THREAD_TASK_MGR_H_
#define _LIS_THREAD_TASK_MGR_H_

#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace LisThread {

typedef std::string TaskId;
// TaskId should be one of the standard type for which hash function specialization is available

enum TaskProcStatus { tpsNone = 0, tpsProcessing = 1, tpsFinished = 2 };

typedef int TaskProcResult;
typedef std::function<void()> TaskStopCallback;
struct TaskProcCtrl {
	bool StopFlag = false; // Flag that indicates if task routine should finish or may continue
	TaskStopCallback StopFunc; // Optional function that should be called by task manager when the task is about to stop
};
typedef void* TaskWorkData;
typedef std::function<TaskProcResult(TaskProcCtrl* proc_ctrl, TaskWorkData work_data)> TaskProc;
typedef std::function<void(TaskProcResult proc_result)> TaskFinCallback;

typedef std::chrono::system_clock::time_point TimeDataType;
enum TimeValueType { tvtStart, tvtFinish };
const auto TimeValue_Empty = std::chrono::system_clock::time_point::min();

class ThreadTaskMgr
{
private:
	struct ThreadTask {
		std::thread* ProcThread = nullptr;
		TaskProcCtrl ProcCtrl;
		TimeDataType ProcStart = TimeValue_Empty, ProcFinish = TimeValue_Empty;
		TaskProcResult ProcResult = 0;
		bool IsProcFinished() { return TimeValue_Empty != ProcFinish; }
	};
	std::unordered_map<TaskId, ThreadTask*> tasks;
	std::mutex task_list_sync;

	bool serviceStopFlag;
	std::thread* serviceThread;
	bool isAutoCleanup;

	ThreadTask* GetTask(const TaskId& task_id, bool auto_create);
	bool StartProc(ThreadTask& task_item,
		TaskProc task_proc, TaskWorkData work_data, TaskFinCallback fin_callback);
	static int WaitProc(ThreadTask& task_item, int wait_time_ms);
	static bool StopProc(ThreadTask& task_item, int wait_time_ms);
	static void ServiceMainProc(ThreadTaskMgr* mgr);
public:
	ThreadTaskMgr(bool auto_cleanup = false);
	~ThreadTaskMgr();
	bool StartTask(const TaskId& task_id, TaskProc task_proc, TaskWorkData work_data,
		TaskFinCallback fin_callback = nullptr);
	bool WaitTask(const TaskId& task_id, int wait_time_ms);
	bool StopTask(const TaskId& task_id);
	TaskProcStatus GetTaskStatus(const TaskId& task_id);
	TimeDataType GetTaskTime(const TaskId& task_id, TimeValueType type);
	bool GetTaskResult(const TaskId& task_id, TaskProcResult& result);
};

} // namespace LisThreadTask

#endif // #ifndef _LIS_THREAD_TASK_MGR_H_
