#pragma once
#include "singleton.h"
#include "common.h"
#include "msgDef.h"
#include "socketService.h"

static const int MAX_QUEUE_LEN = 5000;

class CTaskQueue {
public:
	CTaskQueue();
	void inputCmd(std::shared_ptr<PACKAGE_INFO> & cmd);
	bool getCmd(std::shared_ptr<PACKAGE_INFO> & cmd);	
	void setQuit();
	int getLen();
	bool full();
private:
	std::condition_variable     m_cvCmd;
	std::mutex					m_lockCmd;


	std::list<std::shared_ptr<PACKAGE_INFO> >	m_CmdQ;
	bool m_bQuit;
	
};


template<typename T>
struct SafeQueue : private std::mutex
{
	//static const int wait_infinite = std::numeric_limits<int>::max()();
	static const int wait_infinite = std::numeric_limits<int>::infinity();
	
	SafeQueue(size_t capacity = 0) : capacity_(capacity), exit_(false) {}
	
	bool push(T& v);
	bool pushEx(T& v);	
	T pop_wait(int waitMs = wait_infinite);	
	bool pop_wait(T* v, int waitMs = wait_infinite);
	size_t size();
	void exit();
	bool exited() { return exit_; }
private:
	std::list<T> items_;
	std::condition_variable ready_;
	size_t capacity_;
	std::atomic<bool> exit_;
	void wait_ready(std::unique_lock<std::mutex>& lk, int waitMs);
};


template<typename T> size_t SafeQueue<T>::size() {
	std::lock_guard<std::mutex> lk(*this);
	return items_.size();
}

template<typename T> void SafeQueue<T>::exit() {
	exit_ = true;
	std::lock_guard<std::mutex> lk(*this);
	ready_.notify_all();
}

template<typename T> bool SafeQueue<T>::pushEx(T& v) {
	std::lock_guard<std::mutex> lk(*this);
	if (exit_) {
		return false;
	}
	items_.push_back(std::move(v));
	ready_.notify_all();
	return true;
}

template<typename T> bool SafeQueue<T>::push(T& v) {
	std::lock_guard<std::mutex> lk(*this);
	if (exit_ || (capacity_ && items_.size() >= capacity_)) {
		return false;
	}
	items_.push_back(std::move(v));
	ready_.notify_all();
	return true;
}
template<typename T> void SafeQueue<T>::wait_ready(
	std::unique_lock<std::mutex>& lk, int waitMs)
{
	if (exit_ || !items_.empty()) {
		return;
	}
	if (waitMs == wait_infinite) {
		ready_.wait(lk, [this] { return exit_ || !items_.empty(); });
	}
	else if (waitMs > 0) {
		auto tp = std::chrono::steady_clock::now()
			+ std::chrono::milliseconds(waitMs);
		while (ready_.wait_until(lk, tp) != std::cv_status::timeout
			&& items_.empty() && !exit_) {
		}
	}
}

template<typename T> bool SafeQueue<T>::pop_wait(T* v, int waitMs) {
	std::unique_lock<std::mutex> lk(*this);
	wait_ready(lk, waitMs);
	if (items_.empty()) {
		return false;
	}
	*v = std::move(items_.front());
	items_.pop_front();
	return true;
}

template<typename T> T SafeQueue<T>::pop_wait(int waitMs) {
	std::unique_lock<std::mutex> lk(*this);
	wait_ready(lk, waitMs);
	if (items_.empty()) {
		return T();
	}
	T r = std::move(items_.front());
	items_.pop_front();
	return r;
}

