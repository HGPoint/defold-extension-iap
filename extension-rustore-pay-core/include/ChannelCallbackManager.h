#pragma once

#include <dmsdk/sdk.h>
#include <memory>
#include <vector>
#include <mutex>

namespace RuStoreSDK
{
	struct ChannelCallbackItem
	{
		const char* channel;
		dmScript::LuaCallbackInfo* callback;

		ChannelCallbackItem(const char* ch, dmScript::LuaCallbackInfo* cb) : channel(ch), callback(cb) {}
	};

	class ChannelCallbackManager
	{
		private:
		std::mutex _mutex;
		std::vector<std::shared_ptr<ChannelCallbackItem>> _callbacks;

		public:
		void AddChannelCallback(std::shared_ptr<ChannelCallbackItem> item);
		void ReplaceChannelCallbacks(const char** channels, int channelCount, dmScript::LuaCallbackInfo* callback);
		std::vector<dmScript::LuaCallbackInfo*> FindLuaCallbacksByChannel(const char* channel);

		static ChannelCallbackManager* Instance();
	};
}
