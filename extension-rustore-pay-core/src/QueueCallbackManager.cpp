#include "QueueCallbackManager.h"

using namespace RuStoreSDK;

void QueueCallbackManager::PushQueueCallback(std::shared_ptr<BaseQueueCallbackItem> item)
{
	std::lock_guard<std::mutex> lock(_mutex);

	_queue.push(item);
}

std::queue<std::shared_ptr<BaseQueueCallbackItem>> QueueCallbackManager::GetExexuteQueueCallback(int max)
{
	std::lock_guard<std::mutex> lock(_mutex);

	std::queue<std::shared_ptr<BaseQueueCallbackItem>> executeQueue;
	while (!_queue.empty() && max != 0)
	{
		auto item = _queue.front();
		executeQueue.push(item);
		_queue.pop();
		--max;
	}

	return executeQueue;
}

QueueCallbackManager* QueueCallbackManager::Instance()
{
	static QueueCallbackManager instance;
	
	return &instance;
}
