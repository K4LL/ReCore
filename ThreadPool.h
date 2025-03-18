#pragma once
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <functional>

#include <windows.h>

#include "FlexibleVector.h"

class ThreadGroup {
public:
	std::atomic<int> finishedThreads;
	int              neededThreads;

	void join() {
		int expected = finishedThreads.load(std::memory_order_acquire);
		while (expected < neededThreads) {
			finishedThreads.wait(expected);
			expected = finishedThreads.load(std::memory_order_acquire);
		}
	}
};

class ThreadPool {
private:
	using WorkerThread = HANDLE;
	using Callable = std::function<void()>;

	FlexibleVector<> tasks;

	std::condition_variable assignmentCv;
	std::mutex              assignmentMtx;

	size_t threadsAmmount;
	size_t finishedThreads;

	bool shouldExit;

	static DWORD WINAPI ThreadProc(_In_ LPVOID lpParameter) {
		auto pool = reinterpret_cast<ThreadPool*>(lpParameter);

		Callable task;
		while (true) {
			{
				std::unique_lock<std::mutex> lock(pool->assignmentMtx);
				pool->assignmentCv.wait(lock, [pool]() { return pool->shouldExit || pool->tasks.size() > 0; });
				if (pool->shouldExit) {
					pool->finishedThreads++;
					lock.unlock();
					pool->assignmentCv.notify_all();
					return 14;
				}

				task = std::move(*pool->tasks.at<Callable>(pool->tasks.size() - 1));
				pool->tasks.pop<Callable>();
			}
			task();
		}
	}

	void createThread() {
		CreateThread(nullptr, 0, ThreadProc, this, NULL, NULL);
	}

	void scheduleWorkImpl(const Callable& task) {
		{
			std::lock_guard<std::mutex> lock(this->assignmentMtx);
			this->tasks.push<Callable>(task);
		}
		this->assignmentCv.notify_one();
	}

public:
	ThreadPool() : shouldExit(false), threadsAmmount(0), finishedThreads(0) {}

	~ThreadPool() {
		{
			std::lock_guard<std::mutex> lock(this->assignmentMtx);
			this->shouldExit = true;
		}
		this->assignmentCv.notify_all();

		{
			std::unique_lock<std::mutex> lock(this->assignmentMtx);
			this->assignmentCv.wait(lock, [this]() { return finishedThreads >= threadsAmmount; });
		}
	}

	void build(const size_t threadsAmmount) {
		this->tasks.build<Callable>();
		this->tasks.reserve<Callable>(threadsAmmount);

		this->threadsAmmount = threadsAmmount;

		for (int i = 0; i < threadsAmmount; i++) {
			this->createThread();
		}
	}

	template <typename Fn, typename... Args>
	[[nodiscard]] ThreadGroup* scheduleWork(const size_t threadsAmmount, Fn&& task, Args&&... args) {
		ThreadGroup* group = new ThreadGroup;
		group->neededThreads = threadsAmmount;
		group->finishedThreads.store(0);

		auto wrapper = [task = std::forward<Fn>(task), ...args = std::forward<Args>(args), group]() mutable {
			task(args...);
			group->finishedThreads.fetch_add(1, std::memory_order_release);
			group->finishedThreads.notify_all();
			};

		for (int i = 0; i < threadsAmmount; i++) {
			this->scheduleWorkImpl(wrapper);
		}

		return group;
	}
	template <typename Fn, typename... Args>
	[[nodiscard]] ThreadGroup* scheduleWorkIndexed(const size_t threadsAmmount, Fn&& task, Args&&... args) {
		ThreadGroup* group = new ThreadGroup;
		group->neededThreads = threadsAmmount;
		group->finishedThreads.store(0);

		for (int i = 0; i < threadsAmmount; i++) {
			auto wrapper = [task = std::forward<Fn>(task), ...args = std::forward<Args>(args), i, group]() mutable {
				task(i, args...);
				group->finishedThreads.fetch_add(1, std::memory_order_release);
				group->finishedThreads.notify_all();
				};
			this->scheduleWorkImpl(wrapper);
		}

		return group;
	}
};