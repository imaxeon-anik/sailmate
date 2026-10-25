# Sailmate

Native Sailfish OS AI chat client for OpenAI and Anthropic Claude. Work in progress;
not yet validated on a phone or with live provider credentials.

## Current implementation

- Native Silica provider picker, masked/pasteable API-key entry and connection validation.
- Device-lock-protected, owner-only credentials through Sailfish Secrets; no plaintext fallback.
  Only provider names are indexed in QSettings. Configure a device lock on the phone.
- Streaming text chat, cancel/retry, provider model lists and local SQLite conversation history.
- Pulley actions for new chat, models, previous chats and provider switching.
- Provider changes start a fresh chat. History can only be opened under its original provider.
- Removing a key keeps history and does not revoke the provider-side key.

API usage is billed by each provider separately from consumer chat subscriptions. Model-list
validation does not guarantee billing or access to every chat model. Requests go directly to
OpenAI or Anthropic. Text conversation history is stored unencrypted in the app's private data
directory; do not treat it as an encrypted vault. Messages may remain with the provider under
its retention policies. No analytics, tool execution, third-party gateway or automatic retries.

Responses currently display as plain text, not rendered Markdown. File attachments, MCP,
agent tools, history deletion/export and token-cost reporting are not implemented.

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

Build success does not validate Secrets service/plugin availability, Sailjail identity or
Silica interactions on the actual Xperia. No credentials are included in this repository.

## License

No app redistribution license has been selected yet. The imported development skill under
`.github/skills/sailfishos-app/` retains its separate MIT license and attribution.
