# RuStore Pay SDK for Defold 10.3.1

Practical documentation for integrating payments, one-time purchases, and subscriptions using the RuStore Pay SDK in a Defold project.

> Based on the official RuStore documentation:  
> https://www.rustore.ru/help/sdk/pay/defold/10-3-1

## Table of Contents

- [SDK Features](#sdk-features)
- [Installing the Extensions](#installing-the-extensions)
- [Configuring the Android Manifest](#configuring-the-android-manifest)
- [Configuring Resources](#configuring-resources)
- [Deeplinks and Returning from the Payment App](#deeplinks-and-returning-from-the-payment-app)
- [General Callback Event Workflow](#general-callback-event-workflow)
- [Checking Payment Availability](#checking-payment-availability)
- [Checking User Authorization](#checking-user-authorization)
- [Retrieving Products](#retrieving-products)
- [Retrieving Purchases](#retrieving-purchases)
- [Retrieving a Specific Purchase](#retrieving-a-specific-purchase)
- [Making a Purchase](#making-a-purchase)
- [Guaranteed Two-Step Purchase](#guaranteed-two-step-purchase)
- [Confirming and Cancelling a Two-Step Purchase](#confirming-and-cancelling-a-two-step-purchase)
- [Payment Process Events](#payment-process-events)
- [RuStoreUtils](#rustoreutils)
- [Error Handling](#error-handling)
- [Error Codes](#error-codes)
- [Recommended Integration Flow](#recommended-integration-flow)
- [Issues Found in the Official Example](#issues-found-in-the-official-example)

---

## SDK Features

The main `rustorepay` module provides the following methods:

| Method | Purpose |
|---|---|
| `get_purchase_availability()` | Checks whether payments are available |
| `get_user_authorization_status()` | Checks whether the user is authorized in RuStore |
| `get_products(product_ids)` | Retrieves published products |
| `get_purchases([filter])` | Retrieves the user's purchases |
| `get_purchase(purchase_id)` | Retrieves a purchase by ID |
| `purchase(params, preferred_type, theme, events)` | Starts a purchase with a preferred payment flow |
| `purchase_two_step(params, theme, events)` | Starts a guaranteed two-step purchase |
| `confirm_two_step_purchase(purchase_id, payload)` | Confirms a held purchase |
| `cancel_two_step_purchase(purchase_id)` | Cancels a held purchase |

The `rustorecore` module provides utility methods and the callback event bus.

---

## Installing the Extensions

Download the following archives from the RuStore project release:

- `extension_rustore_pay.zip`
- `extension_rustore_core.zip`

Extract them into the root directory of your Defold project:

```text
your_project/
├── extension_rustore_pay/
│   ├── lib/
│   ├── manifests/
│   ├── src/
│   └── ext.manifest
└── extension_rustore_core/
    ├── include/
    ├── lib/
    ├── manifests/
    ├── src/
    └── ext.manifest
```

Make sure the extension directories are included in the Native Extension build.

---

## Configuring the Android Manifest

The SDK is initialized automatically, but the application parameters must be declared inside `<application>`.

```xml
<meta-data
    android:name="console_app_id_value"
    android:value="@string/rustore_PayClientSettings_consoleApplicationId" />

<meta-data
    android:name="internal_config_key"
    android:value="@string/rustore_PayClientSettings_internalConfigKey" />

<meta-data
    android:name="sdk_pay_scheme_value"
    android:value="@string/rustore_PayClientSettings_deeplinkScheme" />
```

### Main Defold Activity

`DefoldActivity` must use `android:launchMode="singleTask"`:

```xml
<activity
    android:name="com.dynamo.android.DefoldActivity"
    android:exported="true"
    android:launchMode="singleTask"
    android:label="{{project.title}}"
    android:theme="@android:style/Theme.NoTitleBar.Fullscreen"
    android:screenOrientation="{{orientation-support}}"
    android:configChanges="fontScale|keyboard|keyboardHidden|locale|mcc|mnc|navigation|orientation|screenLayout|screenSize|smallestScreenSize|touchscreen|uiMode">

    <meta-data
        android:name="android.app.lib_name"
        android:value="{{exe-name}}" />

    <intent-filter>
        <action android:name="android.intent.action.MAIN" />
        <category android:name="android.intent.category.LAUNCHER" />
    </intent-filter>
</activity>
```

### Deeplink Activity

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

### Internal Pay Activity

```xml
<activity
    android:name="ru.rustore.sdk.pay.internal.presentation.ui.PayActivity"
    android:exported="false"
    android:launchMode="singleTask"
    tools:replace="android:launchMode" />
```

### Permissions

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.WAKE_LOCK" />
```

> The Package Name in `game.project -> Android -> Package` must match the Package Name of the APK or AAB published in RuStore Console.

---

## Configuring Resources

Do not place SDK values directly in the manifest. Create an XML resource file, for example `rustore_values.xml`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <string
        name="rustore_PayClientSettings_consoleApplicationId"
        translatable="false">123456</string>

    <string
        name="rustore_PayClientSettings_internalConfigKey"
        translatable="false">defold</string>

    <string
        name="rustore_PayClientSettings_deeplinkScheme"
        translatable="false">mygame</string>
</resources>
```

Deeplink scheme requirements:

- ASCII characters only;
- must comply with RFC 3986;
- must be unique to the application;
- example: `mygame`, without `://`.

---

## Deeplinks and Returning from the Payment App

The plugin includes `RuStoreIntentFilterActivity`, which:

1. receives an incoming Android `Intent`;
2. passes it to `RuStorePay.INSTANCE.proceedIntent(...)`;
3. returns the user to `com.dynamo.android.DefoldActivity`.

This is required for payments performed through external applications or payment methods such as SberPay or the Faster Payments System (SBP).

Core Activity logic:

```java
@Override
protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    if (savedInstanceState == null) {
        RuStorePay.INSTANCE.proceedIntent(getIntent());
    }

    if (!isTaskRoot()) {
        finish();
        return;
    }

    startGameActivity("com.dynamo.android.DefoldActivity");
    finish();
}

@Override
public void onNewIntent(Intent intent) {
    super.onNewIntent(intent);
    RuStorePay.INSTANCE.proceedIntent(intent);
}
```

---

## General Callback Event Workflow

The SDK returns results asynchronously through `rustorecore.connect` events.

It is recommended to create subscriptions once in `init`:

```lua
function init(self)
    rustorecore.connect("event_name", callback_function)
end
```

Most successful callbacks return a JSON string:

```lua
local data = json.decode(value)
```

Most errors are also returned as JSON:

```lua
local error = json.decode(value)
print(error.simpleName, error.detailMessage)
```

Do not create duplicate subscriptions every time the purchase button is pressed.

---

## Checking Payment Availability

The following conditions are checked:

- monetization is enabled for the company;
- the application is not blocked;
- the user is not blocked.

### Subscription

```lua
function init(self)
    rustorecore.connect(
        "rustore_pay_on_get_purchase_availability_success",
        on_availability_success
    )

    rustorecore.connect(
        "rustore_pay_on_get_purchase_availability_failure",
        on_availability_failure
    )
end

function on_availability_success(self, channel, value)
    local data = json.decode(value)

    if data.isAvailable then
        print("RuStore payments are available")
    else
        print("RuStore payments unavailable", data.cause)
    end
end

function on_availability_failure(self, channel, value)
    local error = json.decode(value)
    print(error.simpleName, error.detailMessage)
end
```

### Call

```lua
rustorepay.get_purchase_availability()
```

---

## Checking User Authorization

Possible values:

- `AUTHORIZED`;
- `UNAUTHORIZED`.

`UNAUTHORIZED` is also returned when the RuStore application is not installed.

```lua
function init(self)
    rustorecore.connect(
        "rustore_pay_on_get_user_authorization_status_success",
        on_auth_success
    )

    rustorecore.connect(
        "rustore_pay_on_get_user_authorization_status_failure",
        on_auth_failure
    )
end

function on_auth_success(self, channel, value)
    if value == RuStorePayEnums.UserAuthorizationStatus.AUTHORIZED then
        print("AUTHORIZED")
    else
        print("UNAUTHORIZED")
    end
end

function on_auth_failure(self, channel, value)
    local error = json.decode(value)
    print(error.simpleName, error.detailMessage)
end

rustorepay.get_user_authorization_status()
```

To open the authorization screen:

```lua
rustorecore.open_rustore_authorization()
```

---

## Retrieving Products

The method returns up to 1,000 products. It can work without user authorization and without the RuStore application being installed.

```lua
local PRODUCT_IDS = {
    "coins_100",
    "remove_ads",
    "premium_month"
}

rustorepay.get_products(PRODUCT_IDS)
```

### Events

```lua
rustorecore.connect(
    "rustore_pay_on_get_products_success",
    on_products_success
)

rustorecore.connect(
    "rustore_pay_on_get_products_failure",
    on_products_failure
)
```

### Processing the Response

```lua
function on_products_success(self, channel, value)
    local products = json.decode(value)

    for _, product in ipairs(products) do
        print(product.productId)
        print(product.type)
        print(product.amountLabel)
        print(product.title)
    end
end
```

### Product Structure

```text
productId        string
type             string
amountLabel      string
price            number | nil
currency         string
title            string
description      string | nil
imageUrl         string
subscriptionInfo array
```

Product types:

- `CONSUMABLE_PRODUCT`;
- `NON_CONSUMABLE_PRODUCT`;
- `SUBSCRIPTION`.

`price` is specified in the smallest currency unit, for example kopecks.

### Subscription Period

```text
simpleName string
duration   string
currency   string | nil
price      number | nil
```

Possible `simpleName` values:

- `TrialPeriod`;
- `PromoPeriod`;
- `MainPeriod`;
- `GracePeriod`;
- `HoldPeriod`.

`duration` is provided in ISO 8601 format.

---

## Retrieving Purchases

Without a filter, the SDK returns the user's purchases with the `PAID` and `CONFIRMED` statuses.

```lua
rustorepay.get_purchases()
```

### Filter

```lua
local filter = {
    productType = RuStorePayEnums.ProductType.CONSUMABLE_PRODUCT,
    purchaseStatus = RuStorePayEnums.ProductPurchaseStatus.PAID
}

rustorepay.get_purchases(filter)
```

Supported filter statuses:

- `PAID`;
- `CONFIRMED`.

### Events

```lua
rustorecore.connect(
    "rustore_pay_on_get_purchases_success",
    on_purchases_success
)

rustorecore.connect(
    "rustore_pay_on_get_purchases_failure",
    on_purchases_failure
)
```

### Main Purchase Fields

```text
purchaseId       string
invoiceId        string
orderId          string | nil
purchaseType     string
status           string
description      string
purchaseTime     string | nil
price            number
amountLabel      string
currency         string
developerPayload string | nil
sandbox          boolean
productId        string
quantity         number
productType      string
```

### Purchase Statuses

| Status | Meaning |
|---|---|
| `INVOICE_CREATED` | An invoice has been created and is awaiting payment |
| `PROCESSING` | Payment is being processed |
| `CANCELLED` | The user cancelled the purchase |
| `REJECTED` | The payment was rejected |
| `EXPIRED` | The payment period expired |
| `PAID` | Funds are held and confirmation is required |
| `CONFIRMED` | The purchase was successfully paid |
| `REFUNDING` | A refund has been initiated |
| `REFUNDED` | The refund was accepted |
| `REVERSED` | The hold was released |

> For a two-step purchase, the `PAID` status does not mean that the product should be granted permanently. The purchase must be confirmed first.

---

## Retrieving a Specific Purchase

```lua
local purchase_id = "purchase-id"
rustorepay.get_purchase(purchase_id)
```

### Events

```lua
rustorecore.connect(
    "rustore_pay_on_get_purchase_success",
    on_purchase_info_success
)

rustorecore.connect(
    "rustore_pay_on_get_purchase_failure",
    on_purchase_info_failure
)
```

```lua
function on_purchase_info_success(self, channel, value)
    local purchase = json.decode(value)
    print(purchase.purchaseId, purchase.status)
end

function on_purchase_info_failure(self, channel, error, id)
    local data = json.decode(error)
    print("Purchase:", id)
    print(data.simpleName, data.detailMessage)
end
```

The method can return a purchase in any status.

---

## Making a Purchase

The `purchase` method lets you specify the preferred purchase flow:

- `ONE_STEP` — immediate charge;
- `TWO_STEP` — attempts to place a hold.

```lua
local params = {
    productId = "coins_100",
    quantity = 1,
    orderId = "order_456",
    developerPayload = "player=123",
    appUserId = "user123",
    appUserEmail = "user@example.com"
}

local json_params = json.encode(params)
local preferred_type = RuStorePayEnums.PreferredPurchaseType.ONE_STEP
local sdk_theme = RuStorePayEnums.SdkTheme.LIGHT
local enable_events = true

rustorepay.purchase(
    json_params,
    preferred_type,
    sdk_theme,
    enable_events
)
```

### Parameters

| Field | Required | Constraints |
|---|---:|---|
| `productId` | Yes | Product ID in RuStore Console |
| `quantity` | No | Defaults to `1`; consumable products only |
| `orderId` | No | Up to 150 characters |
| `developerPayload` | No | Up to 250 characters |
| `appUserId` | No | Up to 128 characters |
| `appUserEmail` | No | Application user's email address |

### Important `TWO_STEP` Behavior

When using `purchase`, the two-step flow is preferred but not guaranteed. The final behavior depends on the payment method selected by the user.

If the payment method does not support holds, the SDK performs a one-step payment.

Before a payment method is selected, `purchaseType` may be `UNDEFINED`.

### Successful Purchase Result

```text
invoiceId    string
orderId      string | nil
productId    string
purchaseId   string
purchaseType string
quantity     number
sandbox      boolean
```

---

## Guaranteed Two-Step Purchase

Use `purchase_two_step` when a hold must be guaranteed. The user will only be shown payment methods that support a two-step flow.

```lua
local params = {
    productId = "coins_100",
    quantity = 1,
    orderId = "order_456",
    developerPayload = "player=123"
}

local json_params = json.encode(params)
local sdk_theme = RuStorePayEnums.SdkTheme.LIGHT
local enable_events = true

rustorepay.purchase_two_step(
    json_params,
    sdk_theme,
    enable_events
)
```

### Events

```lua
rustorecore.connect(
    "rustore_pay_on_purchase_two_step_success",
    on_purchase_two_step_success
)

rustorecore.connect(
    "rustore_pay_on_purchase_two_step_failure",
    on_purchase_two_step_failure
)
```

After a successful hold, the purchase has the `PAID` status. The product should only be granted after server-side validation and successful confirmation, or according to another secure transactional workflow chosen for your application.

---

## Confirming and Cancelling a Two-Step Purchase

### Confirmation

```lua
local purchase_id = "purchase-id"
local developer_payload = "delivery=completed"

rustorepay.confirm_two_step_purchase(
    purchase_id,
    developer_payload
)
```

### Cancellation

```lua
local purchase_id = "purchase-id"
rustorepay.cancel_two_step_purchase(purchase_id)
```

### Recommended Workflow

1. Receive a result with the `PAID` status.
2. Validate the purchase on your server using `invoiceId`.
3. Prepare the product delivery.
4. Call `confirm_two_step_purchase`.
5. Permanently record the delivery after confirmation succeeds.
6. If the product cannot be delivered, call `cancel_two_step_purchase`.

If the purchase is not confirmed within the period defined by RuStore, the hold will be released and the purchase will move to the `REVERSED` status.

---

## Payment Process Events

When `enable_purchase_event_listener = true`, the SDK sends additional payment process events.

The official example uses handlers corresponding to the following stages:

- payment launch;
- successful payment start;
- payment error;
- user cancellation.

Typical handler:

```lua
function on_payment_started(self, channel, value)
    local result = json.decode(value)
    print(result.productId)
    print(result.purchaseId)
    print(result.invoiceId)
end
```

In error and cancellation events, `purchaseId` and `invoiceId` may be absent:

```lua
local purchase_id = result.purchaseId or ""
local invoice_id = result.invoiceId or ""
```

After the payment sheet is cancelled, it is recommended to request the purchase using `get_purchase` when `purchaseId` is known.

---

## RuStoreUtils

### Checking Whether RuStore Is Installed

```lua
local installed = rustorecore.is_rustore_installed()
```

### Opening the Installation Instructions

```lua
rustorecore.open_rustore_download_instruction()
```

### Opening RuStore

```lua
rustorecore.open_rustore()
```

### Opening Authorization

```lua
rustorecore.open_rustore_authorization()
```

If RuStore is not installed, methods that open the application display a Toast indicating that the application could not be opened.

---

## Error Handling

Base structure:

```text
simpleName    string
detailMessage string
```

A network error may additionally contain:

```text
code string | nil
id   string
```

### Generic Handler

```lua
local function handle_rustore_error(error_json)
    local error = json.decode(error_json)

    print("RuStore error:", error.simpleName)
    print("Message:", error.detailMessage)

    if error.simpleName == "RuStorePaymentNetworkException" then
        print("Code:", error.code)
        print("ID:", error.id)

    elseif error.simpleName == "ProductPurchaseCancelled" then
        print("Purchase was cancelled")

    elseif error.simpleName == "ProductPurchaseException" then
        print("Purchase failed")
    end
end
```

### Main Exceptions

| Error | Description |
|---|---|
| `RuStorePaymentNetworkException` | Network or server communication error |
| `RuStorePaymentException` | Base payment SDK error |
| `RuStorePayClientAlreadyExist` | Repeated SDK initialization |
| `RuStorePayClientNotCreated` | A method was called before initialization |
| `RuStorePayInvalidActivePurchase` | Unknown product or purchase type |
| `RuStorePayInvalidConsoleAppId` | Console App ID is missing |
| `RuStorePaySignatureException` | Invalid response signature |
| `EmptyPaymentTokenException` | Payment token was not received |
| `InvalidCardBindingIdException` | Saved card error |
| `ApplicationSchemeWasNotProvided` | Deeplink scheme is missing |
| `ProductPurchaseException` | Purchase error |
| `ProductPurchaseCancelled` | The user closed the payment sheet |
| `RuStoreNotInstalledException` | RuStore is not installed |
| `RuStoreOutdatedException` | The installed RuStore version does not support payments |
| `RuStoreUserUnauthorizedException` | The user is not authorized |
| `RuStoreApplicationBannedException` | The application is blocked |
| `RuStoreUserBannedException` | The user is blocked |

---

## Error Codes

| Code | Description |
|---|---|
| `4000001` | Invalid request or missing required parameter |
| `4000002`, `4000016`, `4040005` | Application not found |
| `4000003` | Application is blocked |
| `4000004` | Application signature does not match |
| `4000005` | Company not found |
| `4000006` | Company is blocked |
| `4000007` | Monetization is disabled or inactive |
| `4000014` | Product not found |
| `4000015` | Product is not published |
| `4000017` | Invalid `quantity` |
| `4000018` | Purchase limit exceeded |
| `4000020` | Product has already been purchased |
| `4000021` | An unfinished purchase exists for this product |
| `4000022` | Purchase not found |
| `4000025` | No suitable payment method is available |
| `4000026` | Invalid purchase type provided for confirmation |
| `4000027` | Invalid purchase status for confirmation |
| `4000028` | Invalid purchase type provided for cancellation |
| `4000029` | Invalid purchase status for cancellation |

---

## Recommended Integration Flow

### On Application Startup

1. Check `rustorecore.is_rustore_installed()`.
2. Call `get_purchase_availability()`.
3. Call `get_user_authorization_status()`.
4. Retrieve the product catalog using `get_products()`.
5. Restore purchases using `get_purchases()`.

### During a Purchase

1. Disable the purchase button to prevent duplicate requests.
2. Generate a unique `orderId`.
3. Pass the internal `appUserId`.
4. Start `purchase` or `purchase_two_step`.
5. Store `purchaseId`, `invoiceId`, and `orderId`.
6. Perform server-side validation.
7. For a two-step flow, confirm or cancel the hold.
8. Grant the product idempotently.
9. Re-enable the interface only after receiving a final result.

### Idempotency

Do not grant a product based only on a local callback. The server should store processed `invoiceId` or `purchaseId` values and prevent duplicate delivery when callbacks are repeated, purchases are restored, or the application is restarted.

---

## Issues Found in the Official Example

The official version 10.3.1 page contains several fragments that appear to be typographical errors or remnants of documentation for another platform:

1. The configuration step mentions `com.godot.game.RuStoreIntentFilterActivity`, while the Defold example below uses `ru.rustore.defold.pay.RuStoreIntentFilterActivity`.
2. One `rustore_values.xml` example uses `internalConfigKey = godot`. For Defold, `defold` is the logical value and is also used elsewhere in the documentation.
3. The `get_purchase()` example omits `purchase_id`, although the method retrieves a specific purchase.
4. The `cancel_two_step_purchase()` example omits `purchase_id`, although a specific purchase must be cancelled.
5. The page contains inconsistent wording about the payment methods that support two-step payments. Verify actual support using the current RuStore documentation and the SDK response.
6. In one example, the `purchase` call with events enabled appears shortened and does not include `sdk_theme`. Use the signature exported by the exact extension version included in your project.

Before release, verify all function signatures against the Lua API actually exported by the version 10.3.1 extension files.

---

## Sources

- Official RuStore Pay SDK documentation for Defold 10.3.1:  
  https://www.rustore.ru/help/sdk/pay/defold/10-3-1
- RuStore Pay SDK documentation for Defold:  
  https://www.rustore.ru/help/sdk/pay/defold
