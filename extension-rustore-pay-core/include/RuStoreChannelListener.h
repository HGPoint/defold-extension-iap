#pragma once

#include <dmsdk/sdk.h>
#include <dmsdk/dlib/android.h>
#include "QueueCallbackManager.h"

namespace RuStoreSDK
{
	class RuStoreChannelListener
	{
	private:
		jobject javaObject = nullptr;
		const char* signature = "Lru/rustore/defold/core/callbacks/IRuStoreChannelListener;";
		
	public:
		jobject GetJWrapper();
		const char* GetSignature();
		
		RuStoreChannelListener();
		~RuStoreChannelListener();
		
		void _OnMessage(JNIEnv* env, jstring jchannel, jstring jvalue);
		void _OnMessage(JNIEnv* env, jstring jchannel, jstring jvalue0, jstring jvalue1);
		static RuStoreChannelListener* Instance();
	};
}
