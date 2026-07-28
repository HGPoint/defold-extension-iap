# Hybrid Payments: Defold IAP + RuStore Pay SDK

This document describes the current hybrid payment implementation in this repository.

The solution is based on the official Defold In-app purchase extension and adds RuStore Pay SDK 10.3.1 support for Android builds distributed in the RU segment. The public game-facing API remains the Defold `iap` module where possible. RuStore is selected internally when the RuStore application is available on the device.

Primary source documents:

- `docs/index.md` - original Defold In-app purchase extension documentation.
- `docs/rustore_pay_defold_10.3.1.en.md` - practical RuStore Pay SDK 10.3.1 documentation for Defold.
- `rustore_pay_defold_10.3.1.md` - Russian version of the RuStore Pay SDK 10.3.1 documentation.

## Objective

The implementation provides a single integration point for game code:

```lua
iap.list(product_ids, callback)
iap.set_listener(listener)
iap.buy(product_id)
iap.finish(transaction)
iap.acknowledge(transaction)
iap.restore()
iap.get_provider_id()
```

On Android, the extension chooses the payment backend at runtime:

| Runtime condition | Backend |
|---|---|
| RuStore is installed | RuStore Pay SDK |
| RuStore is not installed | Existing Defold Android IAP backend, usually Google Play or Amazon |

The intent is to keep existing game-side `iap.*` calls working while routing payments through RuStore on devices where RuStore is available.

## Scope

Implemented in the current solution:

- RuStore Pay SDK 10.3.1 is added to the Android build.
- `extension-rustore-pay-core` is included as a native extension module.
- RuStore callback events are bridged through `extension-rustore-pay-core/src/rustorecore.cpp` into Defold Lua callbacks.
- Android `iap.*` calls in `extension-iap/src/iap_android.cpp` are conditionally redirected to RuStore.
- RuStore product and purchase JSON is converted to fields expected by the Defold IAP Lua API.
- Android manifest entries required by RuStore payments and deeplink return flow are present in `ExtendedAndroidManifest.xml`.

Out of scope for the current implementation:

- iOS, HTML5, Facebook, and Amazon behavior changes.
- A complete public `rustorepay.*` Lua API that mirrors the official RuStore extension.
- Server-side validation implementation.
- Product entitlement/idempotency storage.
- Full subscription lifecycle handling.

## Impacted Modules

| Module | Role |
|---|---|
| `extension-iap/src/iap_android.cpp` | Runtime provider selection and `iap.*` routing. |
| `extension-iap/src/rustorepay.cpp` | JNI bridge from C++ to `ru.rustore.defold.pay.RuStorePay`. |
| `extension-iap/src/rustorepay.h` | C++ declarations for RuStore payment calls. |
| `extension-iap/src/iap.h` | Adds `PROVIDER_ID_RUSTORE = 15`. |
| `extension-rustore-pay-core/src/rustorecore.cpp` | Initializes RuStore core, receives queued SDK events, converts selected events to Defold IAP callback shape. |
| `extension-rustore-pay-core/src/RuStoreChannelListener.cpp` | Native listener receiving Java/Kotlin RuStore channel messages. |
| `extension-rustore-pay-core/src/QueueCallbackManager.cpp` | Thread-safe event queue used to defer SDK callbacks to extension update. |
| `extension-rustore-pay-core/src/ChannelCallbackManager.cpp` | Stores Lua callbacks by RuStore event channel. |
| `extension-rustore-pay-core/src/java/RuStoreJsonConverter.java` | Converts RuStore JSON payloads to Defold IAP-like JSON payloads. |
| `extension-iap/src/java/RuStoreIntentFilterActivity.java` | Handles RuStore deeplink return and forwards intents to `RuStorePay.INSTANCE.proceedIntent(...)`. |
| `extension-iap/manifests/android/ExtendedAndroidManifest.xml` | Required Android manifest configuration for RuStore. |
| `extension-iap/manifests/android/build.gradle` | Adds `ru.rustore.sdk:pay:10.3.1` and support dependencies. |
| `extension-rustore-pay-core/manifests/android/build.gradle` | Adds RuStore Maven repository. |

## Runtime Provider Selection

During Android IAP initialization, `InitializeIAP()` calls `IsRuStoreInstalled()`:

```cpp
g_IAP.m_useOnlyRuStore = dmConfigFile::GetInt(params->m_ConfigFile, "iap.use_only_rustore", 0) == 1;
bool isRuStoreInstalled = IsRuStoreInstalled();
if(g_IAP.m_useOnlyRuStore || isRuStoreInstalled){
    g_IAP.m_isRuStoreInstalled = true;
    GetRuStoreUserAuthorizationStatus();
}
```

If `g_IAP.m_isRuStoreInstalled` is true, the Android implementation redirects selected `iap.*` calls to RuStore.

Important behavior:

- Selection is based on RuStore installation or explicit `iap.use_only_rustore = 1`, not on `android.iap_provider` alone.
- `android.iap_provider = Rustore` is logged but does not instantiate a separate Defold Java IAP provider class.
- If RuStore is installed, `iap.get_provider_id()` returns `PROVIDER_ID_RUSTORE`.
- If RuStore is not installed, the extension uses the existing Google Play or Amazon path.

### RuStore-only mode

The Android pipeline can be forced to RuStore through `game.project`:

```ini
[iap]
use_only_rustore = 1
```

When `iap.use_only_rustore = 1`:

- The public Lua API remains `iap.*`.
- `iap.get_provider_id()` returns `PROVIDER_ID_RUSTORE`.
- Google Play and Amazon Java IAP provider classes are not loaded or instantiated by this extension.
- Defold Android IAP JNI callback object is not created for the fallback provider path.
- `android.iap_provider` is ignored for runtime payment routing.
- No Google Play or Amazon fallback is attempted if RuStore is unavailable, missing, unauthorized, outdated, or misconfigured.

`iap.process_pending_transactions()` also uses `GetRuStorePurchases()` in this mode instead of calling the Defold Android IAP provider.

Default behavior remains auto mode:

```ini
[iap]
use_only_rustore = 0
```

## API Mapping

### `iap.list(product_ids, callback)`

RuStore path:

| Defold IAP | RuStore |
|---|---|
| `iap.list({ ids }, callback)` | `RuStorePay.getProducts(String[])` |
| Product callback success | `rustore_pay_on_get_products_success` |
| Product callback failure | `rustore_pay_on_get_products_failure` |

Implementation flow:

1. `iap.list()` creates a Lua callback from argument 2.
2. The callback is registered for `rustore_pay_on_get_products_success` and `rustore_pay_on_get_products_failure` via `ConnectCallback()`.
3. Product IDs are read from the Lua table and passed to `RuStorePay.getProducts(...)`.
4. `RuStoreJsonConverter.convertProductDetails(...)` converts RuStore product fields to Defold-compatible fields.

Callback behavior:

- Success calls `callback(self, products, nil)`.
- Failure calls `callback(self, nil, error)` with `error.error = "failed to fetch product"`, `error.reason = iap.REASON_UNSPECIFIED`, and `error.rustore_error` containing the raw RuStore error JSON string.

Product field mapping:

| RuStore field | Defold-compatible field |
|---|---|
| `productId` | `ident` |
| `currency` | `currency_code` |
| `amountLabel` | `price_string` |
| `price` in minor units | `price` converted by multiplying by `0.01` |

The original RuStore fields remain in the product table when conversion succeeds.

### `iap.buy(product_id)`

RuStore path:

| Defold IAP | RuStore |
|---|---|
| `iap.buy(product_id)` | `RuStorePay.purchase(params, "ONE_STEP", "LIGHT", false)` by default. |

The purchase flow can be selected through `game.project` without changing the Lua API:

```ini
[iap]
rustore_use_two_step_purchase = 0
```

| Config value | RuStore call |
|---|---|
| `iap.rustore_use_two_step_purchase = 0` or missing | `RuStorePurchase(productId)` / one-step preferred purchase. |
| `iap.rustore_use_two_step_purchase = 1` | `RuStorePurchaseTwoStep(productId)` / guaranteed two-step purchase. |

The current implementation builds purchase params internally:

```json
{
  "productId": "<product_id>",
  "appUserId": "<generated_uuid>",
  "orderId": "<generated_uuid>",
  "quantity": 1,
  "developerPayload": ""
}
```

Current behavior:

- Uses preferred purchase type `ONE_STEP`.
- Uses SDK theme `LIGHT`.
- Disables RuStore purchase process event listener by passing `false`.
- Generates a new UUID for `appUserId` and `orderId` for each purchase call.
- Does not expose custom `developerPayload`, `appUserEmail`, custom quantity, custom order ID, or SDK theme through the public `iap.buy()` call.
- Can switch to guaranteed two-step mode through `iap.rustore_use_two_step_purchase = 1` in `game.project`.
- In one-step mode, restored purchase callbacks are limited to purchases from the last `iap.rustore_one_step_purchase_filter_hour` hours. The default is `24`.

RuStore two-step support exists in C++ through `RuStorePurchaseTwoStep(productId)`, but `iap.buy()` calls `RuStorePurchase(productId)` by default.

### `iap.set_listener(listener)`

RuStore path:

The listener is registered for the following RuStore channels:

| RuStore channel | Intended Defold IAP callback payload |
|---|---|
| `rustore_pay_on_purchase_success` | Purchase transaction table. |
| `rustore_pay_on_purchase_failure` | Failed transaction table. |
| `rustore_pay_on_purchase_two_step_success` | Purchase transaction table. |
| `rustore_pay_on_purchase_two_step_failure` | Failed transaction table. |
| `rustore_pay_on_confirm_two_step_purchase_success` | Confirmed transaction table with `receipt` and `trans_ident` set to `purchaseId`; `ident` is empty because the SDK event contains only `purchaseId`. |
| `rustore_pay_on_confirm_two_step_purchase_failure` | Failed transaction table, except non-confirmable purchase type errors are ignored. |
| `rustore_pay_on_get_purchases_success` | Restored or active purchase transaction table. |
| `rustore_pay_on_get_purchases_failure` | Failed transaction table. |

After registering callbacks, the implementation calls `GetRuStorePurchases()`.

Current `GetRuStorePurchases()` behavior depends on `iap.rustore_use_two_step_purchase`:

| Config | RuStore SDK request | Additional local filter |
|---|---|---|
| `iap.rustore_use_two_step_purchase = 1` | One request with `productType = ""`, `purchaseStatus = "ProductPurchaseStatus.PAID"` | none |
| `iap.rustore_use_two_step_purchase = 0` or missing | Two requests with `productType = ""`, `purchaseStatus = "ProductPurchaseStatus.PAID"` and `purchaseStatus = "ProductPurchaseStatus.CONFIRMED"` | purchase `purchaseTime` must be within the last `iap.rustore_one_step_purchase_filter_hour` hours |

The default one-step local time filter is:

```ini
[iap]
rustore_one_step_purchase_filter_hour = 24
```

Setting `iap.rustore_one_step_purchase_filter_hour = 0` disables the local time filter.

In two-step mode, the implementation explicitly requests `ProductPurchaseStatus.PAID` purchases to stay close to the original Defold Google Play behavior, where `iap.set_listener()` and `iap.restore()` return non-finished active purchases rather than already consumed or completed purchases.

In one-step mode, RuStore SDK is queried separately for `ProductPurchaseStatus.PAID` and `ProductPurchaseStatus.CONFIRMED`. The callback converter only returns purchases from the configured recent time window. Purchases without a parseable `purchaseTime` are excluded when the time filter is enabled. Because the SDK requests are separate, the Lua listener can receive matching purchases in two callback batches.

RuStore `CONFIRMED` two-step purchases are not restored by default. Returning `CONFIRMED` consumables can make game code treat already completed payments as new `TRANS_STATE_PURCHASED` transactions and grant or consume them again unless the game has strict idempotency by `purchaseId` or `invoiceId`.

Repeated `iap.set_listener()` calls replace the previous internal RuStore IAP listener callback registrations for purchase and restore channels instead of accumulating duplicate callbacks.

### `iap.finish(transaction)`

RuStore path:

| Defold IAP | RuStore |
|---|---|
| `iap.finish(transaction)` | `RuStorePay.confirmTwoStepPurchase(transaction.receipt, "")` unless the original RuStore transaction has `purchaseType == "ONE_STEP"`. |

The transaction must contain `state == iap.TRANS_STATE_PURCHASED` and a string `receipt` field.

In the current converter, RuStore `purchaseId` is used as `receipt`, so `iap.finish()` confirms a purchase using `purchaseId` unless the transaction is explicitly marked with the original RuStore field `purchaseType == "ONE_STEP"`.

Current behavior:

- Transactions with `purchaseType == "ONE_STEP"` are ignored with a log message to avoid invalid RuStore confirmation calls.
- Transactions with `purchaseType == "TWO_STEP"`, `purchaseType == "UNDEFINED"`, missing `purchaseType`, or any other value call `confirmTwoStepPurchase()`.
- `rustore_pay_on_confirm_two_step_purchase_success` is forwarded as a purchased transaction table with `status = "CONFIRMED"` and `purchaseType = "TWO_STEP"`. The SDK success event contains only `purchaseId`, so the callback sets `receipt`, `trans_ident`, and `purchaseId` to that value and leaves `ident` empty.
- `rustore_pay_on_confirm_two_step_purchase_failure` is forwarded as a failed transaction, except invalid purchase type errors such as RuStore code `4000026` are logged and ignored because they indicate a non-confirmable purchase type such as `ONE_STEP`.

### `iap.acknowledge(transaction)`

RuStore path:

| Defold IAP | Current RuStore behavior |
|---|---|
| `iap.acknowledge(transaction)` | No-op with a log message. |

RuStore has no direct Google Play acknowledge equivalent in this integration. Confirmation for confirmable RuStore purchases is handled by `iap.finish()` unless the original transaction has `purchaseType == "ONE_STEP"`.

### `iap.restore()`

RuStore path:

Current behavior:

```cpp
if(g_IAP.m_isRuStoreInstalled){
    GetRuStorePurchases();
    lua_pushboolean(L, 1);
    return 1;
}
```

`iap.restore()` returns `true` and requests purchases through `GetRuStorePurchases()`. Purchase callbacks are emitted through the listener previously registered with `iap.set_listener()`.

### `iap.get_provider_id()`

RuStore path:

| Condition | Return value |
|---|---|
| RuStore installed | `PROVIDER_ID_RUSTORE` (`15`) |
| RuStore not installed | Current Google/Amazon provider id |

## Status Mapping

RuStore purchase statuses are converted to Defold transaction states in `RuStoreJsonConverter.purchaseStateToDefoldState(...)`.

| RuStore status/value | Defold state |
|---|---|
| `PROCESSING` | `iap.TRANS_STATE_PURCHASING` (`0`) |
| `PAID` | `iap.TRANS_STATE_PURCHASED` (`1`) |
| `CONFIRMED` | `iap.TRANS_STATE_PURCHASED` (`1`) |
| `Success` | `iap.TRANS_STATE_PURCHASED` (`1`) |
| `Failure` | `iap.TRANS_STATE_FAILED` (`2`) |
| `Cancelled` | `iap.TRANS_STATE_FAILED` (`2`) |
| `CANCELLED` | `iap.TRANS_STATE_FAILED` (`2`) |
| `REJECTED` | `iap.TRANS_STATE_FAILED` (`2`) |
| `EXPIRED` | `iap.TRANS_STATE_FAILED` (`2`) |
| `REFUNDED` | `iap.TRANS_STATE_FAILED` (`2`) |
| `REVERSED` | `iap.TRANS_STATE_FAILED` (`2`) |
| Any other value | `iap.TRANS_STATE_UNVERIFIED` (`4`) |

Documentation note:

For RuStore two-step purchases, `PAID` means funds are held and confirmation is required. It does not mean that the product should be granted permanently. The current mapping treats `PAID` as `TRANS_STATE_PURCHASED`, which is compatible with the Defold IAP shape but can be unsafe for entitlement delivery unless game code performs validation and confirmation before granting permanent content.

## Purchase Field Mapping

For purchase results and restored purchases, RuStore JSON is converted into a Defold-style transaction table.

| RuStore field | Defold-compatible field |
|---|---|
| `productId` | `ident` |
| `purchaseId` | `trans_ident` |
| `purchaseId` | `receipt` |
| `status` | `state` |
| `currency` | `currency_code` |
| `amountLabel` | `price_string` |
| `price` in minor units | `amount` converted by multiplying by `0.01` |
| Full purchase JSON | `original_json` for purchase callback success |

The original RuStore fields, including `purchaseType`, remain in the transaction table when conversion succeeds. The converter also adds `date` using the current device time, not necessarily the RuStore `purchaseTime`.

## Callback Flow

The official RuStore SDK returns results asynchronously through `rustorecore.connect` channels. This implementation uses the same channel mechanism internally and adapts selected channels to Defold IAP callbacks.

Flow:

1. Java/Kotlin RuStore SDK emits a channel event through `RuStoreCore.INSTANCE.emitSignal(...)`.
2. `RuStoreChannelListenerWrapper` forwards the event to native C++.
3. `RuStoreChannelListener.cpp` pushes the event into `QueueCallbackManager`.
4. `rustorecore.cpp` processes the queue during extension update.
5. `ChannelCallbackManager` finds Lua callbacks registered for the event channel.
6. For selected payment channels, `RuStoreJsonConverter` converts the JSON to Defold IAP shape.
7. The Lua callback is called with the converted transaction table.

For unconverted generic RuStore events, callbacks receive the original RuStore channel and value arguments.

## Android Manifest Requirements

The current `extension-iap/manifests/android/ExtendedAndroidManifest.xml` includes the main RuStore requirements from the 10.3.1 documentation.

Main Defold activity:

```xml
<activity
    android:name="com.dynamo.android.DefoldActivity"
    android:exported="true"
    android:launchMode="singleTask"
    ...>
</activity>
```

RuStore deeplink activity:

```xml
<activity
    android:name="ru.rustore.defold.pay.RuStoreIntentFilterActivity"
    android:theme="@android:style/Theme.NoDisplay"
    android:exported="true">
    <intent-filter>
        <action android:name="android.intent.action.VIEW" />
        <category android:name="android.intent.category.DEFAULT" />
        <category android:name="android.intent.category.BROWSABLE" />
        <data android:scheme="@string/rustore_PayClientSettings_deeplinkScheme" />
    </intent-filter>
</activity>
```

RuStore internal pay activity:

```xml
<activity
    android:name="ru.rustore.sdk.pay.internal.presentation.ui.PayActivity"
    android:exported="false"
    android:launchMode="singleTask"
    tools:replace="android:launchMode" />
```

RuStore SDK metadata:

```xml
<meta-data
    android:name="sdk_pay_scheme_value"
    android:value="@string/rustore_PayClientSettings_deeplinkScheme" />

<meta-data
    android:name="console_app_id_value"
    android:value="@string/rustore_PayClientSettings_consoleApplicationId" />

<meta-data
    android:name="internal_config_key"
    android:value="@string/rustore_PayClientSettings_internalConfigKey" />
```

Permissions:

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.WAKE_LOCK" />
<uses-permission android:name="android.permission.POST_NOTIFICATIONS" />
```

A project using this extension must provide Android string resources:

```xml
<resources>
    <string name="rustore_PayClientSettings_consoleApplicationId" translatable="false">123456</string>
    <string name="rustore_PayClientSettings_internalConfigKey" translatable="false">defold</string>
    <string name="rustore_PayClientSettings_deeplinkScheme" translatable="false">mygame</string>
</resources>
```

The deeplink scheme must be ASCII, RFC 3986-compatible, unique to the application, and specified without `://`.

## Android Gradle Requirements

Current dependencies in `extension-iap/manifests/android/build.gradle`:

```gradle
dependencies {
    implementation 'com.android.billingclient:billing:7.0.0'
    implementation "ru.rustore.sdk:pay:10.3.1"
    implementation "com.google.code.gson:gson:2.10.1"
}
```

Current repository declaration in `extension-rustore-pay-core/manifests/android/build.gradle`:

```gradle
repositories {
    google()
    mavenCentral()
    maven {
        url = uri("https://artifactory-external.vkpartner.ru/artifactory/maven-rustore-exposed")
    }
}
```

Build verification should confirm that Defold merges repositories from both native extensions. If repository declarations are not merged as expected, the RuStore SDK dependency may fail to resolve even though the dependency is declared correctly.

## SDK 10.3.1 Signature Check

The connected `RuStoreDefoldPay.jar` exports methods matching the JNI calls used by `rustorepay.cpp`:

| C++ call | Java/Kotlin method exported by jar |
|---|---|
| `getUserAuthorizationStatus()` | `void getUserAuthorizationStatus()` |
| `getPurchaseAvailability()` | `void getPurchaseAvailability()` |
| `getProducts(String[])` | `void getProducts(java.lang.String[])` |
| `getPurchases(String, String)` | `void getPurchases(java.lang.String, java.lang.String)` |
| `getPurchase(String)` | `void getPurchase(java.lang.String)` |
| `purchase(String, String, String, boolean)` | `void purchase(java.lang.String, java.lang.String, java.lang.String, boolean)` |
| `purchaseTwoStep(String, String, boolean)` | `void purchaseTwoStep(java.lang.String, java.lang.String, boolean)` |
| `confirmTwoStepPurchase(String, String)` | `void confirmTwoStepPurchase(java.lang.String, java.lang.String)` |
| `cancelTwoStepPurchase(String)` | `void cancelTwoStepPurchase(java.lang.String)` |
| `proceedIntent(Intent)` | `void proceedIntent(android.content.Intent)` |

This reduces the risk of a direct JNI signature mismatch after migration from RuStore SDK 9 to 10.3.1. The native callback converter for `rustore_pay_on_purchase_product_failure` also calls `RuStoreJsonConverter.convertPurchaseProductFailure(String, String)` with the Java method's two-argument signature. The remaining risks are mainly semantic: event payload shape, purchase status meaning, and entitlement workflow.

## Known Issues And Risks

### Critical

| Issue | Risk | Location |
|---|---|---|
| `PAID` maps to `TRANS_STATE_PURCHASED`. | For two-step purchases, game code may grant a product before `confirmTwoStepPurchase()` succeeds. | `RuStoreJsonConverter.java` |
| `CONFIRMED` purchases are not restored by default. | This matches Defold Google Play non-finished purchase behavior, but non-consumable restore through RuStore may need game-side/server-side handling. | `rustorepay.cpp` |
| `iap.use_only_rustore = 1` disables Google Play and Amazon fallback. | Devices without a working RuStore payment environment cannot recover through the original Defold Android IAP provider. | `iap_android.cpp` |

### Important

| Issue | Risk | Location |
|---|---|---|
| `rustorecore.is_rustore_installed()` is not exposed to Lua. | The official RuStore utility documented for game code is unavailable through Lua. | `rustorecore.cpp` |
| Purchase process events are disabled. | Intermediate payment launch/start/cancel events are not available. | `rustorepay.cpp` |

### Optional Improvements

| Improvement | Benefit |
|---|---|
| Add explicit game/project config to force RuStore, Google Play, or auto mode. | More predictable provider selection for testing and release channels. |
| Expose selected RuStore-specific options through `iap.buy(product_id, options)`. | Allows custom `orderId`, `developerPayload`, `quantity`, and theme. |
| Treat two-step `PAID` as `TRANS_STATE_UNVERIFIED`. | Reduces risk of premature entitlement delivery, but would change the Defold-compatible `iap.finish()` flow. |

## Recommended Entitlement Flow

For production payments, game code should not permanently grant products based only on a local `TRANS_STATE_PURCHASED` callback.

Recommended flow:

1. Receive transaction callback from `iap.set_listener()`.
2. Read `ident`, `receipt`, `trans_ident`, `state`, `purchaseId`, `invoiceId`, and `purchaseType` when present.
3. Send `purchaseId` or `invoiceId` to the game backend for validation.
4. Store processed purchase identifiers server-side to make delivery idempotent.
5. For one-step purchases, grant only after server validation confirms the purchase.
6. For two-step purchases, prepare delivery first, then call confirmation only when delivery can be completed.
7. Grant the product permanently only after confirmation succeeds or after the backend has verified final `CONFIRMED` state.
8. If two-step delivery cannot be completed, cancel the hold using the RuStore cancel flow.

## Manual QA Checklist

Build and environment:

- Android build resolves `ru.rustore.sdk:pay:10.3.1` from the RuStore Maven repository.
- APK/AAB package name matches the RuStore Console package name.
- Required `rustore_PayClientSettings_*` string resources are present in the application.
- `DefoldActivity` uses `singleTask`.
- Deeplink scheme returns the user to the game after external payment flow.

Provider routing:

- Device without RuStore uses Google Play/Amazon path.
- Device with RuStore returns `iap.PROVIDER_ID_RUSTORE` from `iap.get_provider_id()`.
- `iap.use_only_rustore = 1` returns `iap.PROVIDER_ID_RUSTORE` and does not initialize Google Play/Amazon provider classes.
- `iap.use_only_rustore = 1` on a device without a working RuStore payment environment produces RuStore-path unavailable/authorization/purchase errors instead of falling back to Google Play/Amazon.
- Device with RuStore installed but unauthorized produces the expected authorization or purchase failure behavior.

Product listing:

- `iap.list()` returns products with `ident`, `title`, `description`, `currency_code`, `price_string`, and converted `price`.
- `iap.list()` behavior is checked for invalid product IDs and network failure.

Purchase flow:

- Successful one-step purchase emits a transaction callback.
- User-cancelled purchase emits a failed transaction callback.
- Network/server failure emits a failed transaction callback.
- `receipt` and `trans_ident` contain the RuStore `purchaseId`.
- Backend validation can use `purchaseId` or `invoiceId` from the callback payload.

Restore and pending purchases:

- Calling `iap.set_listener()` after app restart returns relevant pending or confirmed purchases.
- Multiple purchases are tested to confirm whether only one callback is emitted.
- `PAID` two-step purchases are not granted permanently before confirmation.

Finish and acknowledge:

- `iap.finish()` is tested on two-step `PAID` purchases.
- `iap.finish()` is tested on one-step purchases to confirm current error behavior.
- `iap.acknowledge()` behavior is tested and documented for the game team.

## Acceptance Criteria For Current Stage

The current hybrid implementation can be considered documented when:

- The Defold-facing API behavior is described through `iap.*` calls.
- RuStore SDK integration points are listed.
- Android manifest and Gradle requirements are documented.
- Status and field mappings are explicit.
- Known risks from RuStore SDK 10.3.1 migration are recorded.
- Manual QA scenarios are defined.

## Recommended Next Implementation Stage

The highest-value technical fix is:

1. Correct the `PAID` two-step mapping so game code can distinguish held purchases from final confirmed purchases without changing the public `iap.*` API.
