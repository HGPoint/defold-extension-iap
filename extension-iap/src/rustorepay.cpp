// #define EXTENSION_NAME RuStorePay
// #define LIB_NAME "RuStorePay"//"iap"
// #define MODULE_NAME "rustorepay"//"iap"
#define DEBUG false

#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)

#include <dmsdk/dlib/android.h>
#include "iap.h"
#include "iap_private.h"
#include <string>
#include <vector>

#include "../../extension-rustore-pay-core/src/rustorecore.h"
#include "AndroidJavaObject.h"

using namespace RuStoreSDK;

// struct IAP
// {
//     IAP()
//     {
//         memset(this, 0, sizeof(*this));
//         m_autoFinishTransactions = true;
//         m_buyStarted = true;
//         m_ProviderId = PROVIDER_ID_RUSTORE;
//     }
//     bool            m_autoFinishTransactions;
//     bool            m_buyStarted;
//     int             m_ProviderId;
// };

// static IAP g_IAP;

static void GetJavaPayInstance(JNIEnv* env, AndroidJavaObject* instance)
{
    jclass cls = dmAndroid::LoadClass(env, "ru.rustore.defold.pay.RuStorePay");
    jfieldID instanceField = env->GetStaticFieldID(cls, "INSTANCE", "Lru/rustore/defold/pay/RuStorePay;");
    jobject obj = env->GetStaticObjectField(cls, instanceField);

    instance->cls = cls;
    instance->obj = obj;
}

int GetRuStoreUserAuthorizationStatus()
{
    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    AndroidJavaObject instance;
    GetJavaPayInstance(env, &instance);
    jmethodID method = env->GetMethodID(instance.cls, "getUserAuthorizationStatus", "()V");
    env->CallVoidMethod(instance.obj, method);
    instance.Free(env);
    
    return 0;
}

static int GetPurchaseAvailability(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    AndroidJavaObject instance;
    GetJavaPayInstance(env, &instance);
    jmethodID method = env->GetMethodID(instance.cls, "getPurchaseAvailability", "()V");
    env->CallVoidMethod(instance.obj, method);
    instance.Free(env);

    return 0;
}

int GetRuStoreProducts(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    std::vector<std::string> productIds;
    if (lua_istable(L, 1))
    {
        int tableSize = lua_objlen(L, 1);
        for (int i = 1; i <= tableSize; i++)
        {
            lua_rawgeti(L, 1, i);
            if (lua_isstring(L, -1))
            {
                const char* productId = lua_tostring(L, -1);
                productIds.push_back(productId);
            }
            lua_pop(L, 1);
        }
    }

    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    jobjectArray jproductIds = env->NewObjectArray(productIds.size(), env->FindClass("java/lang/String"), nullptr);
    for (int i = 0; i < productIds.size(); i++) {
        jstring jproductId = env->NewStringUTF(productIds[i].c_str());
        env->SetObjectArrayElement(jproductIds, i, jproductId);
        env->DeleteLocalRef(jproductId);
    }
    
    AndroidJavaObject instance;
    GetJavaPayInstance(env, &instance);
    jmethodID method = env->GetMethodID(instance.cls, "getProducts", "([Ljava/lang/String;)V");
    env->CallVoidMethod(instance.obj, method, jproductIds);
    instance.Free(env);
    
    env->DeleteLocalRef(jproductIds);
    
    return 0;
}

int GetRuStorePurchases()
{
    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    const char* productType = "";
    const char* purchaseStatus = "PAID";

    jstring jproductType = env->NewStringUTF(productType);
    jstring jpurchaseStatus = env->NewStringUTF(purchaseStatus);

    AndroidJavaObject instance;
    GetJavaPayInstance(env, &instance);
    jmethodID method = env->GetMethodID(instance.cls, "getPurchases", "(Ljava/lang/String;Ljava/lang/String;)V");
    env->CallVoidMethod(instance.obj, method, jproductType, jpurchaseStatus);
    instance.Free(env);

    env->DeleteLocalRef(jproductType);
    env->DeleteLocalRef(jpurchaseStatus);

    return 0;
}

int GetRuStorePurchase(const char* productId)
{
    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    jstring jproductId = env->NewStringUTF(productId);
    
    AndroidJavaObject instance;
    GetJavaPayInstance(env, &instance);
    jmethodID method = env->GetMethodID(instance.cls, "getPurchase", "(Ljava/lang/String;)V");
    env->CallVoidMethod(instance.obj, method, jproductId);
    instance.Free(env);
    
    env->DeleteLocalRef(jproductId);

    return 0;
}

int RuStorePurchase(const char* productId)
{
    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    jclass cls2 = dmAndroid::LoadClass(env, "ru.rustore.defold.pay.RuStoreIntentFilterActivity");
    jmethodID getUUIDMethod = env->GetStaticMethodID(
        dmAndroid::LoadClass(env, "ru.rustore.defold.pay.RuStoreIntentFilterActivity"), 
        "getUUID", 
        "()Ljava/lang/String;"
    );
    jstring juuid = (jstring) env->CallStaticObjectMethod(cls2, getUUIDMethod);
    const char *uuid = env->GetStringUTFChars(juuid, nullptr);

    std::string jsonString = "{ \"productId\":\"" + std::string(productId) + "\", \"appUserId\":\"" + std::string(uuid) + "\", \"orderId\":\"" + std::string(uuid) + "\", \"quantity\":1, \"payload\":\"\" }";
    jstring jparams = env->NewStringUTF(jsonString.c_str());

    const char* preferredPurchaseType = "ONE_STEP";//(char*)luaL_checkstring(L, 2);
    jstring jpreferredPurchaseType = env->NewStringUTF(preferredPurchaseType);
    
    AndroidJavaObject instance;
    GetJavaPayInstance(env, &instance);
    jmethodID method = env->GetMethodID(instance.cls, "purchase", "(Ljava/lang/String;Ljava/lang/String;)V");
    env->CallVoidMethod(instance.obj, method, jparams, jpreferredPurchaseType);
    instance.Free(env);

    env->DeleteLocalRef(jparams);
    env->DeleteLocalRef(jpreferredPurchaseType);

    return 0;
}

int RuStorePurchaseTwoStep(const char* productId)
{
    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    jclass cls2 = dmAndroid::LoadClass(env, "ru.rustore.defold.pay.RuStoreIntentFilterActivity");
    jmethodID getUUIDMethod = env->GetStaticMethodID(
        dmAndroid::LoadClass(env, "ru.rustore.defold.pay.RuStoreIntentFilterActivity"), 
        "getUUID", 
        "()Ljava/lang/String;"
    );
    jstring juuid = (jstring) env->CallStaticObjectMethod(cls2, getUUIDMethod);
    const char *uuid = env->GetStringUTFChars(juuid, nullptr);

    std::string jsonString = "{ \"productId\":\"" + std::string(productId) + "\", \"orderId\":\"" + std::string(uuid) + "\", \"quantity\":1, \"payload\":\"\" }";
    jstring jparams = env->NewStringUTF(jsonString.c_str());

    dmLogInfo("IAP_Buy RuStorePurchaseTwoStep = %s", jsonString.c_str());

    AndroidJavaObject instance;
    GetJavaPayInstance(env, &instance);
    jmethodID method = env->GetMethodID(instance.cls, "purchaseTwoStep", "(Ljava/lang/String;)V");
    env->CallVoidMethod(instance.obj, method, jparams);
    instance.Free(env);

    env->DeleteLocalRef(jparams);

    return 0;
}

int RuStoreConfirmTwoStepPurchase(const char* purchaseId)
{
    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    const char* developerPayload = "";
    
    jstring jpurchaseId = env->NewStringUTF(purchaseId);
    jstring jdeveloperPayload = env->NewStringUTF(developerPayload);

    AndroidJavaObject instance;
    GetJavaPayInstance(env, &instance);
    jmethodID method = env->GetMethodID(instance.cls, "confirmTwoStepPurchase", "(Ljava/lang/String;Ljava/lang/String;)V");
    env->CallVoidMethod(instance.obj, method, jpurchaseId, jdeveloperPayload);
    instance.Free(env);

    env->DeleteLocalRef(jpurchaseId);
    env->DeleteLocalRef(jdeveloperPayload);

    return 0;
}

int RuStoreCancelTwoStepPurchase(const char* purchaseId)
{
    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    jstring jpurchaseId = env->NewStringUTF(purchaseId);

    AndroidJavaObject instance;
    GetJavaPayInstance(env, &instance);
    jmethodID method = env->GetMethodID(instance.cls, "cancelTwoStepPurchase", "(Ljava/lang/String;)V");
    env->CallVoidMethod(instance.obj, method, jpurchaseId);
    instance.Free(env);

    env->DeleteLocalRef(jpurchaseId);

    return 0;
}

#else

#endif
