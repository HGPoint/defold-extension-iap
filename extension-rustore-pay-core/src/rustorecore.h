#if defined(DM_PLATFORM_HTML5) || defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_IOS)

#ifndef RUSTORECORE_H
#define RUSTORECORE_H

#include <dmsdk/sdk.h>
#include <dmsdk/dlib/android.h>
#include "iap_core.h"

static IAPCore g_IAPCore;
static int Connect(lua_State* L);
bool GetCoreAuthorizationStatus();
void CallBackCancelPurchase();
int ConnectCallback(const char* channel, dmScript::LuaCallbackInfo* callback);
int ReplaceCallbacks(const char** channels, int channelCount, dmScript::LuaCallbackInfo* callback);
void SetRuStorePurchaseFilterHours(int hours);
bool IsRuStoreInstalled();

#endif  // RUSTORECORE_H

#endif // DM_PLATFORM_HTML5 || DM_PLATFORM_ANDROID || DM_PLATFORM_IOS
