#include "task_manager.h"

#include <QDateTime>
#include <QThreadPool>
#include <algorithm>
#include <vector>

namespace xequation
{
namespace gui
{

TaskManager::TaskManager(QObject *parent, int max_concurrent_tasks)
	: QObject(parent), max_concurrent_tasks_(std::max(1, max_concurrent_tasks)), thread_pool_(QThreadPool::globalInstance())
{
}

TaskManager::~TaskManager()
{
	Shutdown();

	std::vector<QFutureWatcher<QVariant> *> watchers;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		for (auto &entry : running_tasks_)
		{
			watchers.push_back(entry.second.watcher.get());
		}
	}

	for (auto *watcher : watchers)
	{
		if (watcher)
		{
			watcher->waitForFinished();
		}
	}
}

void TaskManager::EnqueueTask(std::unique_ptr<Task> task, int priority)
{
	if (task->id_.isNull())
	{
		task->id_ = QUuid::createUuid();
	}
	task->create_time_ = QDateTime::currentDateTime();
	task->state_ = Task::State::kPending;

	{
		std::lock_guard<std::mutex> lock(mutex_);
		task_queue_.push(QueuedTask{std::move(task), priority, enqueue_counter_++});
	}

	emit TaskQueued(task->id_);

	MaybeDispatchNext();
}

void TaskManager::CancelTask(const QUuid &task_id)
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto running_it = running_tasks_.find(task_id);
	if (running_it != running_tasks_.end())
	{
		if (running_it->second.task)
		{
			running_it->second.task->state_ = Task::State::kCancelled;
			running_it->second.task->RequestCancel();
		}
		return;
	}

	std::vector<QueuedTask> remaining;
	while (!task_queue_.empty())
	{
		auto queued = std::move(const_cast<QueuedTask &>(task_queue_.top()));
		task_queue_.pop();
		if (queued.task && queued.task->id_ == task_id)
		{
			queued.task->state_ = Task::State::kCancelled;
			queued.task->Cleanup();
			emit queued.task->Cancelled(task_id);
			emit TaskCancelled(task_id);
			continue;
		}
		remaining.push_back(std::move(queued));
	}

	for (auto &queued : remaining)
	{
		task_queue_.push(std::move(queued));
	}
}

void TaskManager::Shutdown()
{
	std::lock_guard<std::mutex> lock(mutex_);

	while (!task_queue_.empty())
	{
		auto queued = std::move(const_cast<QueuedTask &>(task_queue_.top()));
		task_queue_.pop();
		if (queued.task)
		{
			queued.task->state_ = Task::State::kCancelled;
			queued.task->Cleanup();
			emit queued.task->Cancelled(queued.task->id_);
			emit TaskCancelled(queued.task->id_);
		}
	}

	for (auto &entry : running_tasks_)
	{
		if (entry.second.task)
		{
			entry.second.task->RequestCancel();
		}
	}
}

void TaskManager::ClearQueue()
{
	std::lock_guard<std::mutex> lock(mutex_);
	while (!task_queue_.empty())
	{
		auto queued = std::move(const_cast<QueuedTask &>(task_queue_.top()));
		task_queue_.pop();
		if (queued.task)
		{
			queued.task->state_ = Task::State::kCancelled;
			queued.task->Cleanup();
			emit TaskCancelled(queued.task->id_);
			emit TaskCancelled(queued.task->id_);
		}
	}
}

void TaskManager::SetMaxConcurrentTasks(int max_concurrent_tasks)
{
	max_concurrent_tasks_ = std::max(1, max_concurrent_tasks);
	MaybeDispatchNext();
}

int TaskManager::PendingCount() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return static_cast<int>(task_queue_.size());
}

int TaskManager::RunningCount() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return static_cast<int>(running_tasks_.size());
}

bool TaskManager::HasPending() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return !task_queue_.empty();
}

bool TaskManager::IsIdle() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return task_queue_.empty() && running_tasks_.empty();
}

std::vector<QUuid> TaskManager::GetRunningTaskIds() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	std::vector<QUuid> ids;
	ids.reserve(running_tasks_.size());
	for (const auto &entry : running_tasks_)
	{
		ids.push_back(entry.first);
	}
	return ids;
}

QVariant TaskManager::ExecuteTask(Task *task)
{
	if (!task)
	{
		return {};
	}

	task->state_ = Task::State::kRunning;
	emit task->Started(task->id_);
	task->start_time_ = QDateTime::currentDateTime();
	auto result = task->Execute();
	task->end_time_ = QDateTime::currentDateTime();

	if (task->cancel_requested_.load())
	{
		task->state_ = Task::State::kCancelled;
		emit task->Cancelled(task->id_);
	}
	else
	{
		task->state_ = Task::State::kCompleted;
		emit task->Completed(task->id_);
	}

	return result;
}

void TaskManager::MaybeDispatchNext()
{
	while (true)
	{
		std::unique_ptr<Task> task;

		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (task_queue_.empty() ||
				running_tasks_.size() >= static_cast<std::size_t>(max_concurrent_tasks_))
			{
				return;
			}

			auto queued = std::move(const_cast<QueuedTask &>(task_queue_.top()));
			task_queue_.pop();
			task = std::move(queued.task);
		}

		if (!task)
		{
			continue;
		}

		if (task->id_.isNull())
		{
			task->id_ = QUuid::createUuid();
		}

		auto task_id = task->id_;
		auto watcher = std::make_unique<QFutureWatcher<QVariant>>();
		auto watcher_ptr = watcher.get();
		auto task_ptr = task.get();

		QObject::connect(watcher_ptr, &QFutureWatcher<QVariant>::finished, this,
						 [this, task_id]() { OnTaskFinished(task_id); });

		auto future = QtConcurrent::run(thread_pool_, [this, task_ptr]() { return ExecuteTask(task_ptr); });
		watcher_ptr->setFuture(future);

		{
			std::lock_guard<std::mutex> lock(mutex_);
			running_tasks_.emplace(task_id, RunningTaskInfo{std::move(task), std::move(watcher)});
		}

		emit TaskStarted(task_id);
	}
}

void TaskManager::OnTaskFinished(const QUuid &task_id)
{
	std::unique_ptr<Task> task;
	std::unique_ptr<QFutureWatcher<QVariant>> watcher;

	{
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = running_tasks_.find(task_id);
		if (it == running_tasks_.end())
		{
			return;
		}

		task = std::move(it->second.task);
		watcher = std::move(it->second.watcher);
		running_tasks_.erase(it);
	}

	QVariant result;
	if (watcher)
	{
		result = watcher->result();
	}

	if (task)
	{
		if (task->state_ != Task::State::kCancelled)
		{
			task->state_ = Task::State::kCompleted;
		}
		task->Cleanup();

		if (task->state_ == Task::State::kCancelled)
		{
			emit task->Cancelled(task_id);
			emit TaskCancelled(task_id);
		}
		else
		{
			emit task->Completed(task_id);
		}
		emit TaskFinished(task_id, result);
	}

	MaybeDispatchNext();

	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (task_queue_.empty() && running_tasks_.empty())
		{
			emit QueueDrained();
		}
	}
}

} // namespace gui
} // namespace xequation
