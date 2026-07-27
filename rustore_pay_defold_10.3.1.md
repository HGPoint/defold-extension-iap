# RuStore Pay SDK для Defold 10.3.1

Практическая документация по интеграции платежей, разовых покупок и подписок RuStore Pay SDK в проект Defold.

> Основано на официальной документации RuStore:  
> https://www.rustore.ru/help/sdk/pay/defold/10-3-1

## Содержание

- [Возможности SDK](#возможности-sdk)
- [Подключение расширений](#подключение-расширений)
- [Настройка Android Manifest](#настройка-android-manifest)
- [Настройка ресурсов](#настройка-ресурсов)
- [Deeplink и возврат из приложения оплаты](#deeplink-и-возврат-из-приложения-оплаты)
- [Общий принцип работы с callback-событиями](#общий-принцип-работы-с-callback-событиями)
- [Проверка доступности платежей](#проверка-доступности-платежей)
- [Проверка авторизации](#проверка-авторизации)
- [Получение продуктов](#получение-продуктов)
- [Получение покупок](#получение-покупок)
- [Получение конкретной покупки](#получение-конкретной-покупки)
- [Выполнение покупки](#выполнение-покупки)
- [Гарантированная двухстадийная покупка](#гарантированная-двухстадийная-покупка)
- [Подтверждение и отмена двухстадийной покупки](#подтверждение-и-отмена-двухстадийной-покупки)
- [События процесса оплаты](#события-процесса-оплаты)
- [RuStoreUtils](#rustoreutils)
- [Обработка ошибок](#обработка-ошибок)
- [Коды ошибок](#коды-ошибок)
- [Рекомендуемый сценарий интеграции](#рекомендуемый-сценарий-интеграции)
- [Замеченные несоответствия официального примера](#замеченные-несоответствия-официального-примера)

---

## Возможности SDK

Основной модуль `rustorepay` предоставляет методы:

| Метод | Назначение |
|---|---|
| `get_purchase_availability()` | Проверка доступности платежей |
| `get_user_authorization_status()` | Проверка авторизации пользователя в RuStore |
| `get_products(product_ids)` | Получение опубликованных продуктов |
| `get_purchases([filter])` | Получение списка покупок пользователя |
| `get_purchase(purchase_id)` | Получение покупки по ID |
| `purchase(params, preferred_type, theme, events)` | Покупка с предпочтительным типом оплаты |
| `purchase_two_step(params, theme, events)` | Гарантированная двухстадийная покупка |
| `confirm_two_step_purchase(purchase_id, payload)` | Подтверждение холдированной покупки |
| `cancel_two_step_purchase(purchase_id)` | Отмена холдированной покупки |

Модуль `rustorecore` предоставляет вспомогательные методы и шину callback-событий.

---

## Подключение расширений

Скачайте из релиза проекта RuStore следующие архивы:

- `extension_rustore_pay.zip`
- `extension_rustore_core.zip`

Распакуйте их в корень проекта Defold:

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

Проверьте, что каталоги расширений попадают в сборку Native Extension.

---

## Настройка Android Manifest

SDK инициализируется автоматически, но параметры приложения должны быть объявлены внутри `<application>`.

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

### Основная Activity Defold

Для `DefoldActivity` требуется `android:launchMode="singleTask"`:

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

### Activity для deeplink

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

### Внутренняя Pay Activity

```xml
<activity
    android:name="ru.rustore.sdk.pay.internal.presentation.ui.PayActivity"
    android:exported="false"
    android:launchMode="singleTask"
    tools:replace="android:launchMode" />
```

### Разрешения

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.WAKE_LOCK" />
```

> Package Name в `game.project -> Android -> Package` должен совпадать с Package Name APK/AAB, опубликованного в RuStore Консоли.

---

## Настройка ресурсов

Не задавайте значения SDK непосредственно в манифесте. Создайте XML-файл ресурсов, например `rustore_values.xml`:

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

Требования к deeplink scheme:

- только ASCII;
- формат должен соответствовать RFC 3986;
- схема должна быть уникальной для приложения;
- пример: `mygame`, без `://`.

---

## Deeplink и возврат из приложения оплаты

Плагин содержит `RuStoreIntentFilterActivity`, которая:

1. получает входящий Android `Intent`;
2. передает его в `RuStorePay.INSTANCE.proceedIntent(...)`;
3. возвращает пользователя в `com.dynamo.android.DefoldActivity`.

Это необходимо для оплаты через внешние приложения и способы оплаты, например SberPay или СБП.

Основная логика Activity:

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

## Общий принцип работы с callback-событиями

SDK возвращает результаты асинхронно через события `rustorecore.connect`.

Подписки рекомендуется создавать один раз в `init`:

```lua
function init(self)
    rustorecore.connect("event_name", callback_function)
end
```

Большинство успешных callback возвращают JSON-строку:

```lua
local data = json.decode(value)
```

Большинство ошибок также приходят как JSON:

```lua
local error = json.decode(value)
print(error.simpleName, error.detailMessage)
```

Не создавайте повторные подписки при каждом нажатии кнопки покупки.

---

## Проверка доступности платежей

Проверяются следующие условия:

- у компании подключена монетизация;
- приложение не заблокировано;
- пользователь не заблокирован.

### Подписка

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

### Вызов

```lua
rustorepay.get_purchase_availability()
```

---

## Проверка авторизации

Возможные значения:

- `AUTHORIZED`;
- `UNAUTHORIZED`.

`UNAUTHORIZED` также возвращается, если приложение RuStore не установлено.

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

Для открытия авторизации:

```lua
rustorecore.open_rustore_authorization()
```

---

## Получение продуктов

Метод возвращает до 1000 продуктов. Он может работать без авторизации пользователя и без установленного приложения RuStore.

```lua
local PRODUCT_IDS = {
    "coins_100",
    "remove_ads",
    "premium_month"
}

rustorepay.get_products(PRODUCT_IDS)
```

### События

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

### Обработка ответа

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

### Структура продукта

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

Типы продукта:

- `CONSUMABLE_PRODUCT`;
- `NON_CONSUMABLE_PRODUCT`;
- `SUBSCRIPTION`.

`price` указывается в минимальных единицах валюты, например в копейках.

### Период подписки

```text
simpleName string
duration   string
currency   string | nil
price      number | nil
```

Значения `simpleName`:

- `TrialPeriod`;
- `PromoPeriod`;
- `MainPeriod`;
- `GracePeriod`;
- `HoldPeriod`.

`duration` передается в формате ISO 8601.

---

## Получение покупок

Без фильтра SDK возвращает покупки пользователя в статусах `PAID` и `CONFIRMED`.

```lua
rustorepay.get_purchases()
```

### Фильтр

```lua
local filter = {
    productType = RuStorePayEnums.ProductType.CONSUMABLE_PRODUCT,
    purchaseStatus = RuStorePayEnums.ProductPurchaseStatus.PAID
}

rustorepay.get_purchases(filter)
```

Поддерживаемые статусы фильтра:

- `PAID`;
- `CONFIRMED`.

### События

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

### Основные поля покупки

```text
purchaseId      string
invoiceId       string
orderId         string | nil
purchaseType    string
status          string
description     string
purchaseTime    string | nil
price           number
amountLabel     string
currency        string
developerPayload string | nil
sandbox         boolean
productId       string
quantity        number
productType     string
```

### Статусы покупки

| Статус | Значение |
|---|---|
| `INVOICE_CREATED` | Создан счет, ожидается оплата |
| `PROCESSING` | Оплата выполняется |
| `CANCELLED` | Пользователь отменил покупку |
| `REJECTED` | Оплата отклонена |
| `EXPIRED` | Истекло время оплаты |
| `PAID` | Средства захолдированы, требуется подтверждение |
| `CONFIRMED` | Покупка успешно оплачена |
| `REFUNDING` | Запущен возврат |
| `REFUNDED` | Возврат принят |
| `REVERSED` | Холд отменен |

> Для двухстадийной покупки статус `PAID` не означает окончательную выдачу товара. Сначала необходимо подтвердить покупку.

---

## Получение конкретной покупки

```lua
local purchase_id = "purchase-id"
rustorepay.get_purchase(purchase_id)
```

### События

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

Метод может вернуть покупку в любом статусе.

---

## Выполнение покупки

Метод `purchase` позволяет указать предпочтительную стадийность:

- `ONE_STEP` — списание сразу;
- `TWO_STEP` — попытка холдирования.

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

### Параметры

| Поле | Обязательное | Ограничения |
|---|---:|---|
| `productId` | Да | ID продукта в RuStore Консоли |
| `quantity` | Нет | По умолчанию `1`, только для consumable |
| `orderId` | Нет | До 150 символов |
| `developerPayload` | Нет | До 250 символов |
| `appUserId` | Нет | До 128 символов |
| `appUserEmail` | Нет | Email пользователя приложения |

### Важное поведение `TWO_STEP`

В `purchase` двухстадийность является предпочтительной, но не гарантируется. Итог зависит от выбранного способа оплаты.

Если способ оплаты не поддерживает холдирование, SDK выполняет одностадийную оплату.

До выбора способа оплаты `purchaseType` может иметь значение `UNDEFINED`.

### Результат успешной покупки

```text
invoiceId   string
orderId     string | nil
productId   string
purchaseId  string
purchaseType string
quantity    number
sandbox     boolean
```

---

## Гарантированная двухстадийная покупка

Для гарантированного холдирования используйте `purchase_two_step`. Пользователю будут показаны только способы оплаты, поддерживающие двухстадийную схему.

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

### События

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

После успешного холда покупка имеет статус `PAID`. Товар следует выдавать только после серверной проверки и успешного подтверждения, либо по выбранной вами безопасной транзакционной схеме.

---

## Подтверждение и отмена двухстадийной покупки

### Подтверждение

```lua
local purchase_id = "purchase-id"
local developer_payload = "delivery=completed"

rustorepay.confirm_two_step_purchase(
    purchase_id,
    developer_payload
)
```

### Отмена

```lua
local purchase_id = "purchase-id"
rustorepay.cancel_two_step_purchase(purchase_id)
```

### Рекомендуемая логика

1. Получить результат со статусом `PAID`.
2. Проверить покупку на сервере по `invoiceId`.
3. Подготовить выдачу товара.
4. Вызвать `confirm_two_step_purchase`.
5. После успеха подтверждения окончательно зафиксировать выдачу.
6. При невозможности выдать товар вызвать `cancel_two_step_purchase`.

Если покупка не подтверждена в установленный RuStore срок, холд будет отменен и покупка перейдет в `REVERSED`.

---

## События процесса оплаты

При `enable_purchase_event_listener = true` SDK отправляет дополнительные события процесса.

В официальном примере используются обработчики, соответствующие этапам:

- запуск платежа;
- успешное начало оплаты;
- ошибка оплаты;
- отмена пользователем.

Типичный обработчик:

```lua
function on_payment_started(self, channel, value)
    local result = json.decode(value)
    print(result.productId)
    print(result.purchaseId)
    print(result.invoiceId)
end
```

В событиях ошибки и отмены `purchaseId` и `invoiceId` могут отсутствовать:

```lua
local purchase_id = result.purchaseId or ""
local invoice_id = result.invoiceId or ""
```

После отмены платежной шторки рекомендуется дополнительно запросить покупку через `get_purchase`, если известен `purchaseId`.

---

## RuStoreUtils

### Проверка установки RuStore

```lua
local installed = rustorecore.is_rustore_installed()
```

### Открытие инструкции по установке

```lua
rustorecore.open_rustore_download_instruction()
```

### Открытие RuStore

```lua
rustorecore.open_rustore()
```

### Открытие авторизации

```lua
rustorecore.open_rustore_authorization()
```

Если RuStore не установлен, методы открытия приложения показывают Toast с ошибкой открытия.

---

## Обработка ошибок

Базовая структура:

```text
simpleName    string
detailMessage string
```

Сетевая ошибка дополнительно может содержать:

```text
code string | nil
id   string
```

### Универсальный обработчик

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

### Основные исключения

| Ошибка | Описание |
|---|---|
| `RuStorePaymentNetworkException` | Ошибка сети или серверного взаимодействия |
| `RuStorePaymentException` | Базовая ошибка платежного SDK |
| `RuStorePayClientAlreadyExist` | Повторная инициализация SDK |
| `RuStorePayClientNotCreated` | Метод вызван до инициализации |
| `RuStorePayInvalidActivePurchase` | Неизвестный тип продукта/покупки |
| `RuStorePayInvalidConsoleAppId` | Не задан Console App ID |
| `RuStorePaySignatureException` | Некорректная подпись ответа |
| `EmptyPaymentTokenException` | Не получен платежный токен |
| `InvalidCardBindingIdException` | Ошибка сохраненной карты |
| `ApplicationSchemeWasNotProvided` | Не задан deeplink scheme |
| `ProductPurchaseException` | Ошибка покупки |
| `ProductPurchaseCancelled` | Пользователь закрыл платежную шторку |
| `RuStoreNotInstalledException` | RuStore не установлен |
| `RuStoreOutdatedException` | Версия RuStore не поддерживает платежи |
| `RuStoreUserUnauthorizedException` | Пользователь не авторизован |
| `RuStoreApplicationBannedException` | Приложение заблокировано |
| `RuStoreUserBannedException` | Пользователь заблокирован |

---

## Коды ошибок

| Код | Описание |
|---|---|
| `4000001` | Некорректный запрос или обязательный параметр |
| `4000002`, `4000016`, `4040005` | Приложение не найдено |
| `4000003` | Приложение заблокировано |
| `4000004` | Подпись приложения не совпадает |
| `4000005` | Компания не найдена |
| `4000006` | Компания заблокирована |
| `4000007` | Монетизация отключена или неактивна |
| `4000014` | Продукт не найден |
| `4000015` | Продукт не опубликован |
| `4000017` | Некорректный `quantity` |
| `4000018` | Превышен лимит покупок |
| `4000020` | Продукт уже приобретен |
| `4000021` | Есть незавершенная покупка продукта |
| `4000022` | Покупка не найдена |
| `4000025` | Нет подходящего способа оплаты |
| `4000026` | Для подтверждения передан неверный тип покупки |
| `4000027` | Неверный статус покупки для подтверждения |
| `4000028` | Для отмены передан неверный тип покупки |
| `4000029` | Неверный статус покупки для отмены |

---

## Рекомендуемый сценарий интеграции

### При запуске приложения

1. Проверить `rustorecore.is_rustore_installed()`.
2. Вызвать `get_purchase_availability()`.
3. Вызвать `get_user_authorization_status()`.
4. Получить каталог через `get_products()`.
5. Восстановить покупки через `get_purchases()`.

### При покупке

1. Заблокировать кнопку повторной покупки.
2. Сформировать уникальный `orderId`.
3. Передать внутренний `appUserId`.
4. Запустить `purchase` или `purchase_two_step`.
5. Сохранить `purchaseId`, `invoiceId`, `orderId`.
6. Провести серверную валидацию.
7. Для двухстадийной схемы подтвердить или отменить холд.
8. Выдать товар идемпотентно.
9. Разблокировать интерфейс только после финального результата.

### Идемпотентность

Не выдавайте товар только по локальному callback. Сервер должен хранить обработанные `invoiceId` или `purchaseId` и исключать повторную выдачу при повторном callback, восстановлении покупки или перезапуске приложения.

---

## Замеченные несоответствия официального примера

В официальной странице версии 10.3.1 встречаются фрагменты, которые выглядят как опечатки или остатки документации другой платформы:

1. В тексте шага настройки упоминается Activity `com.godot.game.RuStoreIntentFilterActivity`, хотя Defold-пример ниже использует `ru.rustore.defold.pay.RuStoreIntentFilterActivity`.
2. В одном примере `rustore_values.xml` указан `internalConfigKey = godot`. Для Defold логично и в другом фрагменте документации указано значение `defold`.
3. В примере вызова `get_purchase()` отсутствует `purchase_id`, хотя метод получает сведения о конкретной покупке.
4. В примере вызова `cancel_two_step_purchase()` отсутствует `purchase_id`, хотя отменяется конкретная покупка.
5. В описании поддерживаемых способов двухстадийной оплаты на странице встречаются разные формулировки. Фактическую поддержку следует проверять по актуальной документации RuStore и результату SDK.
6. В одном примере `purchase` с включенными событиями аргументы выглядят сокращенными и не содержат `sdk_theme`; используйте сигнатуру, соответствующую подключенной версии расширения.

Перед релизом рекомендуется сверить сигнатуры функций с Lua API, фактически экспортируемым файлами расширения версии 10.3.1.

---

## Источники

- Официальная документация RuStore Pay SDK для Defold 10.3.1:  
  https://www.rustore.ru/help/sdk/pay/defold/10-3-1
- Раздел Pay SDK для Defold:  
  https://www.rustore.ru/help/sdk/pay/defold
