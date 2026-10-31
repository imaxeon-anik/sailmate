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
- Conversation pulley actions: change model, update API key and Home; the push-up menu at the
  end of a chat offers change model and Home. History offers
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

Responses currently display as plain text, not rendered Markdown. General file attachments, MCP,
agent tools, general chat export and token-cost reporting are not implemented.

Long-press a history row and choose **Delete chat** to open a confirmation. Deletion removes
only that provider's selected local thread, including its stored photos. It does not remove
other chats, profiles, saved API keys, or provider-side data. History rows show the last saved
message activity time, right-aligned in a smaller font: **hh:mm** for today, **d MMM** earlier
this year and **MMM yyyy** for older chats. Reading a
chat does not change its timestamp. Help shows the app version immediately below its heading.

## Photo attachments

The chat composer offers **Attach photo** using the native Sailfish image picker. Attach one
JPEG or PNG per message, add a text caption, and remove or replace the preview before sending.
The selected model and endpoint must support image input; model discovery does not establish
that capability. OpenAI and Custom requests use image_url blocks; Anthropic uses base64 image
source blocks. Unsupported requests show an error rather than silently dropping the image.

Images are processed off the UI thread, limited to 10 MiB and 20 megapixels at input, resized
to at most 1600 pixels per side, and re-encoded as JPEG without source metadata. Transparency
is flattened on white. Encoded image data is limited to 1,400,000 characters per image and
8 MiB across the conversation sent to a provider. Text-only conversations retain their format.

Successful turns save the processed image in the local SQLite history, so reopening and
continuing a chat includes its photos even if the original file is removed. The image column
is added without replacing existing tables or changing settings/credential storage. Photos are
stored unencrypted like chat text and are sent again as context when continuing the chat.
Failed turns retain their image for explicit retry; provider switching clears pending photos.
The Pictures Sailjail permission is required. GIF, HEIC, camera capture and multiple photos
in a single message are not supported in this first version.

Native picker behavior, Qt image plugins, and real provider image requests still require
SDK compilation and device testing. No photos are sent merely by selecting them.

## Custom OpenAI-compatible profiles

Choose **Custom** in the provider picker to add a named profile with an HTTPS base URL
(including `/v1` if required) and, optionally, a fixed model ID. Each profile has its own key
and chat history; your key and messages go to that endpoint, so check it before sending.

## Screen previews

Open [screen previews](docs/sailmate-preview.html) in a browser for twelve illustrative UI
mockups, including the Custom profile flow and centred pulley-menu labels. These are not screenshots of the running
Sailfish app; native styling and interactions still require device validation.

## Building

Sailmate builds with the [Sailfish SDK](https://docs.sailfishos.org/Tools/Sailfish_SDK/)
(tested with SDK 3.13 and a SailfishOS 5.1 target; the code stays compatible with Qt 5.6 and
C++11). From the repository root:

```bash
sfdk -c target=SailfishOS-5.1.0.11-aarch64 build
```

The RPM is written to `RPMS/`. Use the `armv7hl` or `i486` target for other devices or the
emulator. Install it on a device with developer mode enabled:

```bash
devel-su pkcon install-local harbour-sailmate-*.aarch64.rpm
```

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
