#include "RuStoreChannelListener.h"

#if defined(DM_PLATFORM_ANDROID)

using namespace RuStoreSDK;

RuStoreChannelListener::RuStoreChannelListener()
{
    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    jclass javaClass = dmAndroid::LoadClass(env, "ru.rustore.defold.core.wrappers.RuStoreChannelListenerWrapper");
    jmethodID constructor = env->GetMethodID(javaClass, "<init>", "(J)V");
    
    javaObject = env->NewObject(javaClass, constructor, (jlong)this);
    javaObject = env->NewGlobalRef(javaObject);

    thread.Detach();
}

RuStoreChannelListener::~RuStoreChannelListener()
{
    if (javaObject != nullptr)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();

        jclass cls = dmAndroid::LoadClass(env, "ru.rustore.defold.core.wrappers.RuStoreChannelListenerWrapper");
        jmethodID method = env->GetMethodID(cls, "disposeCppPointer", "()V");
        
        env->CallVoidMethod(cls, method);

        env->DeleteGlobalRef(javaObject);

        thread.Detach();
    }
}

jobject RuStoreChannelListener::GetJWrapper()
{
    return javaObject;
}

const char* RuStoreChannelListener::GetSignature()
{
    return signature;
}

void RuStoreChannelListener::_OnMessage(JNIEnv* env, jstring jchannel, jstring jvalue)
{
    const char* channel = env->GetStringUTFChars(jchannel, 0);
    const char* value = env->GetStringUTFChars(jvalue, 0);

    auto item = std::make_shared<QueueCallbackItem>((std::string(channel)), (std::string(value)));
    QueueCallbackManager::Instance()->PushQueueCallback(item);

    env->ReleaseStringUTFChars(jchannel, channel);
    env->ReleaseStringUTFChars(jvalue, value);
}

void RuStoreChannelListener::_OnMessage(JNIEnv* env, jstring jchannel, jstring jvalue0, jstring jvalue1)
{
    const char* channel = env->GetStringUTFChars(jchannel, 0);
    const char* value0 = env->GetStringUTFChars(jvalue0, 0);
    const char* value1 = env->GetStringUTFChars(jvalue1, 0);

    auto item = std::make_shared<QueueCallbackItemTwoParams>(std::string(channel), std::string(value0), std::string(value1));
    QueueCallbackManager::Instance()->PushQueueCallback(item);

    env->ReleaseStringUTFChars(jchannel, channel);
    env->ReleaseStringUTFChars(jvalue0, value0);
    env->ReleaseStringUTFChars(jvalue1, value1);
}

RuStoreChannelListener* RuStoreChannelListener::Instance()
{
    static RuStoreChannelListener instance;

    return &instance;
}

extern "C"
{
    JNIEXPORT void JNICALL Java_ru_rustore_defold_core_wrappers_RuStoreChannelListenerWrapper_nativeOnMessage(JNIEnv* env, jobject obj, jlong pointer, jstring channel, jstring value)
    {
        auto castobj = reinterpret_cast<RuStoreChannelListener*>(pointer);
        castobj->_OnMessage(env, channel, value);
    }

    JNIEXPORT void JNICALL Java_ru_rustore_defold_core_wrappers_RuStoreChannelListenerWrapper_nativeOnMessageTwoParams(JNIEnv* env, jobject obj, jlong pointer, jstring channel, jstring value0, jstring value1)
    {
        auto castobj = reinterpret_cast<RuStoreChannelListener*>(pointer);
        castobj->_OnMessage(env, channel, value0, value1);
    }
}

#endif
