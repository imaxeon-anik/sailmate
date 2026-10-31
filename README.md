# Sailmate

Native Sailfish OS AI chat client for OpenAI, Anthropic Claude and custom OpenAI-compatible
endpoints, using your own API keys. Tested on an Xperia 10 V running SailfishOS 5.1.

## Features

- Streaming chat with cancel and retry, model selection and local chat history per provider.
- API keys stored with Sailfish Secrets behind the device lock; no plaintext fallback.
- Photo attachments: one JPEG or PNG per message, resized and stripped of metadata before
  sending. The chosen model must support images.
- Long-press a chat in history to delete it.
- Native Silica UI with pull-down and push-up menus, and Help in the app.

Not implemented: Markdown rendering, file attachments other than photos, tools/MCP and chat export.

## Privacy and billing

Requests go directly to the provider or custom endpoint you choose, and API usage is billed by
that provider (consumer chat subscriptions usually don't include API access). Chat history and
photos are stored unencrypted in the app's private data directory. No analytics.

## Custom OpenAI-compatible profiles

Choose **Custom** in the provider picker to add a named profile with an HTTPS base URL
(including `/v1` if required) and, optionally, a fixed model ID. Each profile has its own key
and chat history; your key and messages go to that endpoint, so check it before sending.

## Screen previews

[Screen previews](docs/sailmate-preview.html) are illustrative HTML mockups, not screenshots.

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

## License

No app redistribution license has been selected yet. The imported development skill under
`.github/skills/sailfishos-app/` retains its separate MIT license and attribution.
