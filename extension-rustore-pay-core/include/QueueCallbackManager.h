#pragma once

#include <queue>
#include <mutex>

namespace RuStoreSDK
{
	struct BaseQueueCallbackItem {
		const std::string channel;
		virtual ~BaseQueueCallbackItem() = default;

	protected:
		BaseQueueCallbackItem(const std::string& channel) : channel(channel) {}
	};

	struct QueueCallbackItem : public BaseQueueCallbackItem {
		const std::string value;

		QueueCallbackItem(const std::string& channel, const std::string& value)
		: BaseQueueCallbackItem(channel), value(value) {}
	};

	struct QueueCallbackItemTwoParams : public BaseQueueCallbackItem {
		const std::string value0;
		const std::string value1;

		QueueCallbackItemTwoParams(const std::string& channel, const std::string& value0, const std::string& value1)
		: BaseQueueCallbackItem(channel), value0(value0), value1(value1) {}
	};

	class QueueCallbackManager
	{
	private:
		std::mutex _mutex;
		std::queue<std::shared_ptr<BaseQueueCallbackItem>> _queue;

	public:
		void PushQueueCallback(std::shared_ptr<BaseQueueCallbackItem> item);
		std::queue<std::shared_ptr<BaseQueueCallbackItem>> GetExexuteQueueCallback(int max = -1);
		
		static QueueCallbackManager* Instance();
	};
}
