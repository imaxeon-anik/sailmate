# Sailmate

Native Sailfish OS AI chat client for OpenAI, Anthropic Claude and named custom
OpenAI-compatible API profiles. Work in progress;
not yet validated on a phone or with live provider credentials.

## Current implementation

- Native Silica provider picker, masked/pasteable API-key entry and connection validation.
- Device-lock-protected, owner-only credentials through Sailfish Secrets; no plaintext fallback.
  Provider identifiers and non-secret custom profile metadata are stored in QSettings.
  Configure a device lock on the phone.
- Streaming text chat, cancel/retry, provider model lists and local SQLite conversation history.
- User messages appear in right-positioned, theme-tinted bubbles; AI replies stay on the left
  with more width. Both keep left-aligned body text and visible speaker labels.
- Chat history is the parent page: native back navigation from a conversation returns there.
  Connecting opens a fresh conversation with Home and history beneath it, including for Custom
  profiles. Swipe back from history to Home, or use its Home pulley action. Returning Home
  does not delete history or send it to another endpoint. History entries push conversations.
- The composer uses a native themed send icon beside the message field, changing to cancel
  while a request runs. Empty messages cannot be sent; Retry appears separately after errors.
- Conversation pulley actions: new chat, change model and Home. History offers
  new chat and Home, which returns to provider selection. Back navigation is disabled while a request is active; cancel
  or wait before leaving so a streaming response is not orphaned.
- Provider changes start a fresh chat. History can only be opened under its original provider.
- Home's Help pulley action opens the messages and provider-switching information, with native back navigation.
- Removing a key keeps history and does not revoke the provider-side key.

API usage is billed by each provider separately from consumer chat subscriptions. Model-list
validation does not guarantee billing or access to every chat model. Requests go directly to
OpenAI, Anthropic or the selected custom endpoint. Text conversation history is stored unencrypted in the app's private data
directory; do not treat it as an encrypted vault. Messages may remain with the provider under
its retention policies. No analytics, tool execution, built-in third-party gateway or automatic retries.

Responses currently display as plain text, not rendered Markdown. File attachments, MCP,
agent tools, individual chat deletion/export and token-cost reporting are not implemented.

## Custom OpenAI-compatible profiles

Choose **Custom** in the provider picker, then pull down **Add profile**. Give each profile
an identifiable name and an HTTPS base URL, including `/v1` if the service requires it.
The list shows all saved profiles, or "No custom profiles" with a pull-down hint when empty.
Selecting a profile opens its local chat history without loading a key or contacting the endpoint.
Opening or starting a Custom chat is local and does not load credentials or contact the endpoint.
Use **Connect** on the chat page before sending. It checks Sailfish Secrets directly, including
when the saved-key index is missing, and opens key setup only when no secret exists. Key setup
returns to the same conversation with its draft intact. **Update API key** is in the chat menu.
Model discovery can be cancelled with the chat's cancel icon; saved messages remain readable.
Reconnecting preserves an existing conversation's model. If it is no longer available, sending
is disabled until you explicitly choose a replacement using **Choose model**.
Swipe back from Custom history to the profile list, or use Home to return to provider selection.
Sailmate appends `/models` for discovery and `/chat/completions` for streaming text chat.
Only Bearer API-key authentication and streaming OpenAI-compatible chat completions are
supported. HTTP, URL user information, query strings and fragments are rejected. Azure
deployment URLs, query-based authentication and custom headers are not supported.

Leave the manual model ID empty to discover the service's models; custom model names are
not filtered to OpenAI's naming conventions. Alternatively, enter the service's exact model
ID to bypass `/models`. **Save and continue** then saves the key without validation: it is
**not authentication verification**, and does not establish model access or billing status.
Those are only tested when a chat request is made. “Key saved” indicates storage, not proof
that the credential is valid.

Key-index write failures are reported instead of being treated as successful saves. The key-entry
page also offers **Recover saved key**, which loads the existing secret and repairs its settings
index when possible. This does not recover missing Custom profile metadata or change secret ownership.

History initially lists 200 conversations; **Load older chats** reveals subsequent batches.
If a received response cannot be saved, **Retry save** retries only the local database write,
without another API request. Navigation and new sends are blocked until it succeeds or you
confirm **Discard unsaved changes**. The confirmation offers **Copy conversation** before
discarding; copying places private conversation text on the system clipboard. Discarding clears
only the in-memory conversation, never its previously saved history. The unsaved response is
still in memory: force-closing the app or losing power can discard it.

Profiles are persisted when created, even if key entry is cancelled. Names, URLs and manual
models are immutable; profile editing is not implemented. Create a separate
profile for endpoint or model-configuration changes. Each profile has its own immutable
identity, saved key and conversation history, even when names or URLs are the same. Removing
a key through the profile's long-press menu retains the profile and history and does not revoke
the key at the service. The delete icon opens a confirmation to permanently delete the profile,
its saved key and all its local chats. Other profiles are unaffected. Key deletion must succeed
before profile/history deletion proceeds; if a later step fails, an error explains what to retry.
These operations do not revoke provider-side keys or delete data held by the service.
Switching profiles never transfers the previous profile's history.

All request data, including the API key and conversation sent for
completion, goes to the chosen URL; its operator's access, billing and retention rules apply.
Check the endpoint shown in the profile list, key-entry page and chat header before sending.

## Screen previews

Open [screen previews](docs/sailmate-preview.html) in a browser for twelve illustrative UI
mockups, including the Custom profile flow and centred pulley-menu labels. These are not screenshots of the running
Sailfish app; native styling and interactions still require device validation.

## SSD development

Source: `/Volumes/ANIKT9-VMs/sailfish-dev/src/sailmate`.
Build VM: `sfbuild`, Ubuntu x86_64 with Sailfish SDK 3.13.5.
Target: SailfishOS 5.1.0.11-aarch64 (Qt 5.6.3 / C++11).

```bash
/Volumes/ANIKT9-VMs/sailfish-dev/vm-start.sh
bash scripts/build-rpm.sh aarch64
```

VS Code: run **Sailmate: build Xperia RPM (aarch64)** or Cmd+Shift+B.
The script synchronizes only this app into `~/src/sailmate` in the VM and copies RPMs to
`/Volumes/ANIKT9-VMs/sailfish-dev/xperia10v/app-rpms/`. VM-only source edits are overwritten.
The YouTube project and its build script are not modified.

Never disconnect the SSD while the VM runs; never run the VM on two Macs simultaneously.
Stop with `/Volumes/ANIKT9-VMs/sailfish-dev/vm-stop.sh` before ejecting the SSD.
Phone installation is a separate, explicitly authorized action.

## Tests

The backend unit tests use fake replies and an in-memory database: no API keys, external
requests or charges. After synchronizing source via the build script, inside the Ubuntu VM:

```bash
mkdir -p ~/sailmate-tests
cd ~/sailmate-tests
qmake ~/src/sailmate/tests/network/network.pro
make -j2
./tst_chatcontroller
```

Host test dependencies: `qtbase5-dev`, `libqt5sql5-sqlite`, `g++`, `make`.
Ubuntu host tests use Qt 5.15; the RPM build separately checks Qt 5.6 compatibility.
Tests cover split/UTF-8 SSE frames, malformed/error events, incomplete output, credential
validation, cancellation, provider-bound history and request-state guards.

## Device acceptance checks still required

1. Launch from the launcher under Sailjail; confirm no QML errors and a readable themed UI.
2. Connect each provider with a real API key; check rejected-key, offline and quota errors.
3. Restart the app and confirm saved keys can be loaded through Sailfish Secrets. Check
   device lock/unlock, denied/cancelled interaction and key deletion/reconnection.
4. Send/cancel/retry a streaming response, switch models, reopen history and change providers;
   confirm previous-provider history is not sent to another provider.
5. Check keyboard overlap, orientation, long replies, scroll position and background/resume.
6. Create multiple custom profiles (including duplicate names and arbitrary model IDs), verify
   displayed endpoints and isolated keys/history, and exercise invalid URL/name/model errors.
   Test discovery and manual-model flows separately: manual mode must save without a network
   validation request. Verify cancellation, key removal and restart retain profiles/history.
7. Select a Custom profile while offline and confirm its history list appears. Test new and
  existing chat opening with saved, missing and rejected keys. Cancel profile deletion, then
  confirm deletion removes only that profile's key/chats and remains deleted after restart.
  Deny Secrets deletion and verify the profile/history remain; retry after unlocking.

Build success does not validate Secrets service/plugin availability, Sailjail identity or
Silica interactions on the actual Xperia. No credentials are included in this repository.

## License

No app redistribution license has been selected yet. The imported development skill under
`.github/skills/sailfishos-app/` retains its separate MIT license and attribution.
