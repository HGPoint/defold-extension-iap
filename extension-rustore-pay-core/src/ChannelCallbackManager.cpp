#include "ChannelCallbackManager.h"

using namespace RuStoreSDK;

void ChannelCallbackManager::AddChannelCallback(std::shared_ptr<ChannelCallbackItem> item)
{
	std::lock_guard<std::mutex> lock(_mutex);

	_callbacks.push_back(item);
}

std::vector<dmScript::LuaCallbackInfo*> ChannelCallbackManager::FindLuaCallbacksByChannel(const char* channel)
{
	std::lock_guard<std::mutex> lock(_mutex);

	std::vector<dmScript::LuaCallbackInfo*> result;

	for (const auto& callback : _callbacks)
	{
		if (std::strcmp(callback->channel, channel) == 0)
		{
			result.push_back(callback->callback);
		}
	}

	return result;
}

ChannelCallbackManager* ChannelCallbackManager::Instance()
{
	static ChannelCallbackManager instance;

	return &instance;
}
