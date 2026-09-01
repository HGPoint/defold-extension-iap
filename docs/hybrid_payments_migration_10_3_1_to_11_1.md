# Hybrid Payments Migration: RuStore Pay 10.3.1 to 11.1.0

This document defines the migration plan for the hybrid payment integration in this repository.

## Objective

Upgrade the RuStore Pay SDK used by the existing hybrid implementation from 10.3.1 to 11.1.0 while preserving the current Defold `iap.*` API, provider routing, callback conversion, and Google Play/Amazon fallback behavior.

The migration must combine:

- the current repository-specific hybrid implementation;
- compatible changes from the official RuStore Defold extension 11.1.0;
- the documented SDK changes introduced in 11.0.0 and 11.1.0.

## Mandatory Migration Rule

The current solution must not be replaced with the upstream RuStore extension.

The following operations are prohibited:

- copying the upstream `rustorepay.cpp` over the current hybrid bridge;
- copying the upstream `rustorecore.cpp` over the current callback adapter;
- replacing the current `iap_android.cpp` provider routing with the standalone RuStore implementation;
- removing the current `RuStoreJsonConverter.java` adapter;
- replacing the current Android manifest or Gradle files wholesale.

Every change must be made on top of the current hybrid implementation and reviewed against the upstream 11.1.0 source as a reference. Upstream files may be used selectively only when their behavior is compatible with the existing architecture.

## Scope

### In scope

- Upgrade RuStore Pay runtime artifacts to SDK 11.1.0.
- Update JNI calls whose signatures changed in SDK 11.x.
- Preserve the `iap.*` public contract and runtime provider selection.
- Preserve the callback queue and Defold transaction conversion.
- Carry forward SDK 11.0 purchase acknowledgement data where it is available.
- Add or update tests and static checks for the changed contracts.
- Update the hybrid payment documentation and migration history.

### Out of scope

- Replacing the hybrid API with a public standalone `rustorepay.*` integration.
- Changing iOS, HTML5, Facebook, Amazon, or Google Play behavior unrelated to this migration.
- Implementing server-side purchase validation or entitlement storage.
- Changing the two-step entitlement policy unless explicitly approved as a separate change.
- Adding first-purchase coupon UI or game-facing coupon configuration. SDK 11.1 support is obtained through the updated SDK artifact.

## Current Architecture Baseline

The current implementation consists of:

- `extension-iap/src/iap_android.cpp`: provider selection and `iap.*` routing;
- `extension-iap/src/rustorepay.cpp`: JNI calls into `RuStorePay`;
- `extension-rustore-pay-core/src/rustorecore.cpp`: callback queue processing and hybrid callback adaptation;
- `extension-rustore-pay-core/src/java/RuStoreJsonConverter.java`: RuStore-to-Defold transaction conversion;
- `extension-iap/src/java/RuStoreIntentFilterActivity.java`: deeplink handling and UUID generation used by the current bridge;
- the existing Android manifest and fallback Google Billing dependency.

This architecture is the source of truth for the migration. The upstream extension is not the source of truth for repository-specific behavior.

## Upstream 11.1.0 Changes To Incorporate

According to the official Defold SDK history:

- SDK Pay is upgraded to 11.1.0.
- The public upstream API is unchanged in 11.1.0.
- First-purchase coupon support is added to the payment sheet.

Changes from 11.0.0 that affect integration compatibility:

- `getPurchases` receives a third `acknowledgementState` argument.
- `updateAcknowledgementState(purchaseId, state, developerPayload)` is added.
- `getBillingSubscriptions()` is added.
- Purchase models expose `acknowledgementState`.
- The Maven repository changes to `https://nexus-external.vkteam.ru/repository/maven-rustore-exposed`.

The upstream 11.1.0 archive must be checked for artifact and source inconsistencies. In particular, the supplied upstream `build.gradle` currently declares `pay:11.0.0`; the migration must use the explicitly verified `11.1.0` artifact.

## Migration Stages

### Stage 1: Capture baseline

- Record the current repository status and relevant diff before editing.
- Verify the current 10.3.1 JAR method signatures with `javap`.
- Record current hybrid behavior for listing, buying, restore, finish, acknowledgement, and provider fallback.
- Keep the existing unmodified implementation available as the regression baseline.

Acceptance criteria:

- The baseline signatures and behavior are documented.
- No unrelated worktree changes are modified.

### Stage 2: Update Android artifacts and repositories

- Replace only the RuStore Pay and Core JAR files with the verified 11.1.0 release artifacts.
- Change the Pay dependency to `ru.rustore.sdk:pay:11.1.0`.
- Use the Nexus RuStore Maven repository in the Pay module.
- Review the Core repository declaration separately; do not copy the upstream Core Gradle file without verifying repository resolution.
- Preserve `com.android.billingclient:billing:8.3.0` for the fallback provider.
- Preserve project-specific manifest attributes and permissions unless a verified SDK requirement requires a focused change.

Acceptance criteria:

- Both JARs resolve and are included in the Android build.
- The final dependency graph contains RuStore Pay 11.1.0 and the required fallback billing dependency.

### Stage 3: Adapt JNI contracts

- Update the current `getPurchases` JNI lookup from `(String, String)` to `(String, String, String)`.
- Pass an empty acknowledgement-state value for existing internal restore requests unless the current hybrid API explicitly supplies a filter.
- Preserve the current purchase, confirmation, cancellation, authorization, and product JNI behavior.
- Decide separately whether `updateAcknowledgementState` and `getBillingSubscriptions` need internal bridge functions. Do not expose them through `iap.*` without an explicit API decision.
- Keep `getUUID()` compatibility, or replace it with an equivalent implementation before removing the method from the Activity.

Acceptance criteria:

- Every JNI method lookup matches the 11.1.0 JAR.
- No current hybrid call depends on an upstream method that was removed.
- Google Play/Amazon paths remain unchanged.

### Stage 4: Preserve and extend callback conversion

- Keep `RuStoreJsonConverter.java` in the project.
- Preserve all existing Defold fields: `ident`, `state`, `receipt`, `trans_ident`, `currency_code`, `price_string`, `amount`, `date`, and `original_json`.
- Preserve current `PAID`/`CONFIRMED` filtering behavior and the configured confirmed-purchase time window.
- Copy `acknowledgementState` from RuStore purchase JSON when present.
- Add handling for SDK 11.x statuses such as `EXECUTING` and `REFUNDING` without changing existing entitlement semantics unless separately approved.
- Ensure the converter remains packaged in the final APK; it is not supplied by the upstream Core JAR.

Acceptance criteria:

- Existing callback payloads remain backward-compatible.
- New SDK fields do not overwrite Defold fields incorrectly.
- Two-step `PAID` transactions remain distinguishable from final `CONFIRMED` transactions.

### Stage 5: Verify deeplink and manifest behavior

- Compare the current Activity and the upstream 11.1 Activity.
- Preserve `singleTask` on `DefoldActivity`.
- Preserve the RuStore deeplink Activity and PayActivity declarations.
- Verify that `proceedIntent` remains callable with the current one-argument signature, or make a focused change if the verified SDK requires otherwise.
- Verify that UUID generation used by current purchases still exists after any Activity change.

Acceptance criteria:

- External payment flows return to `DefoldActivity`.
- No Activity class or JNI helper is removed accidentally.

### Stage 6: Regression verification

Run static checks and an Android build, then perform device tests for:

- no RuStore installed: Google Play/Amazon fallback;
- RuStore installed and authorized: provider routing and product listing;
- RuStore-only mode;
- one-step purchase success and cancellation;
- guaranteed two-step purchase, confirmation, and cancellation;
- restore/pending purchases in both configured modes;
- repeated listener registration;
- product and purchase conversion including `acknowledgementState`;
- deeplink return from an external payment application;
- `iap.finish()` and the intentional RuStore `iap.acknowledge()` behavior.

## Files Expected To Be Affected

Expected implementation files:

- `extension-iap/lib/android/RuStoreDefoldPay.jar`;
- `extension-rustore-pay-core/lib/android/RuStoreDefoldCore.jar`;
- `extension-iap/manifests/android/build.gradle`;
- `extension-rustore-pay-core/manifests/android/build.gradle`, if repository resolution requires it;
- `extension-iap/src/rustorepay.cpp`;
- `extension-rustore-pay-core/src/rustorecore.cpp`, only for focused compatibility changes;
- `extension-rustore-pay-core/src/java/RuStoreJsonConverter.java`;
- `extension-iap/src/java/RuStoreIntentFilterActivity.java`, only if required by the verified SDK contract;
- `docs/hybrid_payments.md`.

No upstream standalone extension directory should be added to the repository as part of this migration.

## Risks And Mitigations

| Risk | Mitigation |
|---|---|
| Runtime `NoSuchMethodError` from `getPurchases` | Verify JAR signatures and update the current JNI descriptor together with the artifact. |
| Loss of hybrid callback conversion | Do not replace `rustorecore.cpp` or `RuStoreJsonConverter.java` wholesale. |
| Purchase crash after removing `getUUID()` | Preserve the helper or migrate UUID generation before changing the Activity. |
| Premature entitlement for two-step `PAID` purchases | Preserve explicit `PAID` status and existing entitlement warning. |
| RuStore dependency cannot resolve | Verify the Nexus repository in a real Android build. |
| Fallback billing regression | Keep Billing 8.3.0 and test a build without RuStore. |
| Silent data loss for new purchase fields | Extend conversion tests and retain original RuStore JSON fields. |

## Rollout And Rollback

The migration is Android-only and should be delivered as one reviewable change after all stages pass.

Rollback consists of restoring the previous 10.3.1 JARs, dependency version, repository configuration, and the focused JNI changes. The current hybrid source must remain easy to compare with the migration diff; no destructive replacement is allowed.

## Final Acceptance Criteria

- The runtime RuStore Pay SDK is exactly 11.1.0.
- The current hybrid `iap.*` behavior is preserved.
- JNI descriptors match the installed JARs.
- `acknowledgementState` is not lost when supplied by SDK 11.x.
- The converter and callback queue remain repository-owned.
- Google Play/Amazon fallback behavior is preserved.
- Documentation describes the architecture rule, migration history, new fields, and verification results.
