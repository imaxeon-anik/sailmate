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
  Connecting opens a fresh conversation with history beneath it. History entries push conversations.
- The composer uses a native themed send icon beside the message field, changing to cancel
  while a request runs. Empty messages cannot be sent; Retry appears separately after errors.
- Conversation pulley actions: new chat, change model and change provider. History offers
  new chat and change provider. Back navigation is disabled while a request is active; cancel
  or wait before leaving so a streaming response is not orphaned.
- Provider changes start a fresh chat. History can only be opened under its original provider.
- Removing a key keeps history and does not revoke the provider-side key.

API usage is billed by each provider separately from consumer chat subscriptions. Model-list
validation does not guarantee billing or access to every chat model. Requests go directly to
OpenAI, Anthropic or the selected custom endpoint. Text conversation history is stored unencrypted in the app's private data
directory; do not treat it as an encrypted vault. Messages may remain with the provider under
its retention policies. No analytics, tool execution, built-in third-party gateway or automatic retries.

Responses currently display as plain text, not rendered Markdown. File attachments, MCP,
agent tools, history deletion/export and token-cost reporting are not implemented.

## Custom OpenAI-compatible profiles

Choose **Custom** in the provider picker, then pull down **Add profile**. Give each profile
an identifiable name and an HTTPS base URL, including `/v1` if the service requires it.
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

Profiles are persisted when created, even if key entry is cancelled. Names, URLs and manual
models are immutable; profile editing and deletion are not implemented. Create a separate
profile for endpoint or model-configuration changes. Each profile has its own immutable
identity, saved key and conversation history, even when names or URLs are the same. Removing
a key retains the profile and history and does not revoke the key at the service. Switching
profiles starts a new chat and never transfers the previous profile's history.

Use only trusted endpoints and obtain your organisation's permission before entering a work
API key or conversation. All request data, including the API key and conversation sent for
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

Build success does not validate Secrets service/plugin availability, Sailjail identity or
Silica interactions on the actual Xperia. No credentials are included in this repository.

## License

No app redistribution license has been selected yet. The imported development skill under
`.github/skills/sailfishos-app/` retains its separate MIT license and attribution.
