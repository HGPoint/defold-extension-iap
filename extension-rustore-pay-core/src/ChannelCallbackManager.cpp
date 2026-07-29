#include "ChannelCallbackManager.h"

#include <cstring>

using namespace RuStoreSDK;

void ChannelCallbackManager::AddChannelCallback(std::shared_ptr<ChannelCallbackItem> item)
{
	std::lock_guard<std::mutex> lock(_mutex);

	_callbacks.push_back(item);
}

void ChannelCallbackManager::ReplaceChannelCallbacks(const char** channels, int channelCount, dmScript::LuaCallbackInfo* callback)
{
	std::lock_guard<std::mutex> lock(_mutex);
	std::vector<dmScript::LuaCallbackInfo*> removedCallbacks;
	std::vector<std::shared_ptr<ChannelCallbackItem>> keptCallbacks;

	for (const auto& existing : _callbacks)
	{
		bool replace = false;
		for (int i = 0; i < channelCount; i++)
		{
			if (std::strcmp(existing->channel, channels[i]) == 0)
			{
				replace = true;
				break;
			}
		}

		if (replace)
		{
			bool alreadyRemoved = false;
			for (const auto& removed : removedCallbacks)
			{
				if (removed == existing->callback)
				{
					alreadyRemoved = true;
					break;
				}
			}

			if (!alreadyRemoved)
			{
				removedCallbacks.push_back(existing->callback);
			}
		}
		else
		{
			keptCallbacks.push_back(existing);
		}
	}

	for (int i = 0; i < channelCount; i++)
	{
		keptCallbacks.push_back(std::make_shared<ChannelCallbackItem>(channels[i], callback));
	}

	_callbacks.swap(keptCallbacks);

	for (const auto& removed : removedCallbacks)
	{
		dmScript::DestroyCallback(removed);
	}
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
