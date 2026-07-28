#if defined(DM_PLATFORM_HTML5) || defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_IOS)

#ifndef RUSTOREPAY_H
#define RUSTOREPAY_H

#include <dmsdk/sdk.h>
#include <dmsdk/dlib/android.h>

int GetRuStoreUserAuthorizationStatus();
int GetRuStorePurchases(const char* purchaseStatus);
int GetRuStoreProducts(lua_State* L);
int GetRuStorePurchase(const char* productId);
int RuStorePurchase(const char* productId);
int RuStorePurchaseTwoStep(const char* productId);
int RuStoreConfirmTwoStepPurchase(const char* purchaseId);
int RuStoreCancelTwoStepPurchase(const char* purchaseId);

#endif  // RUSTOREPAY_H

#endif // DM_PLATFORM_HTML5 || DM_PLATFORM_ANDROID || DM_PLATFORM_IOS
