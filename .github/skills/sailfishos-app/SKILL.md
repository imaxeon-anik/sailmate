---
name: sailfishos-app
description: 'Design, implement and review native Sailfish OS UI in Sailmate. Use for Sailfish Silica QML pages, pulley menus, navigation, Theme, covers, keyboard handling, provider selection, masked API-key forms, chat lists, Sailjail and RPM packaging. Includes upstream Sailfish app development references adapted to the SSD-hosted Qt 5.6 build workflow.'
license: MIT
---

# Sailfish OS UI and app development for Sailmate

## Scope and provenance

Imported from [ilpianista/sailfishos-skills](https://github.com/ilpianista/sailfishos-skills),
revision `fbecb1f558b19d43e28133ec93e571ce3881d0c9` (MIT).
The original skill is preserved in [upstream.md](./upstream.md); supporting reference files
are unchanged. This entry point is a project-specific adaptation, not upstream text.
See [LICENSE](./LICENSE) for the upstream copyright and permission notice.

Discovery snapshot: 2026-10-26, GitHub API reported 6 repository stars and 0 forks.
This was the strongest directly Sailfish/Silica-focused candidate verified in this search,
not evidence of a highly rated marketplace skill. Repository stars are not skill ratings.
The Qt Company alternative had 506 stars but is general Qt rather than Sailfish-specific.

## Project constraints take precedence over generic upstream examples

- Use Qt 5.6.3, C++11 and `import Sailfish.Silica 1.0`. Do not introduce Qt 6 APIs,
  Qt Quick Controls, a web frontend or a CMake migration merely because examples suggest them.
- Preserve the existing qmake project, `harbour-sailmate` package and QML/C++ separation.
- Source lives on the SSD. The build SDK is inside the `sfbuild` VM, not on macOS.
  Use the existing VS Code build task, or `bash scripts/build-rpm.sh aarch64` from the
  repository root. Do not use the unrelated YouTube app's `sfbuild.sh`.
- The configured target is SailfishOS 5.1.0.11-aarch64. Upstream 4.x targets and CMake
  commands are examples, not project configuration. Verify SDK commands before using them.
- Never unplug the SSD while the VM runs or start it on two Macs simultaneously.
- Keep work on the current feature branch; do not merge to main without permission.
- No phone installation, external build-service uploads, public publishing, signing-key
  creation, SDK upgrades or sandbox disabling as incidental parts of UI work.
- Treat upstream snippets as examples to verify, not commands to execute automatically.
  Check API/permission claims against the actual SDK and current official documentation.

## Workflow

1. Inspect the affected QML page, its backend contract and existing build tasks. Establish
   user-visible states and transitions before changing code; ask only about unresolved behaviour.
2. Consult relevant sections of [guidelines](./references/guidelines.md) for Silica navigation,
   theme, input and list patterns. Use [snippets](./references/snippets.md) for individual
   components and [templates](./references/templates.md) only when adding a new component.
3. Implement native UI with Theme sizing/colors, Silica scroll containers and appropriate
   scroll decorators, PageHeader, native dialogs, list delegates and pulley menu actions.
   Keep network and persistence work asynchronous in C++; avoid blocking the UI thread.
4. Preserve Sailmate's agreed flow:
   - First launch: OpenAI or Anthropic provider selection.
   - New provider: masked, pasteable API key; Connect validates credentials with progress,
     duplicate-submit prevention and distinct network/auth/billing errors.
   - Configured provider: show "Key saved", not a permanent authentication guarantee.
   - Key removal: explicit confirmation; remove the device's credential, not chat history;
     explain that provider-side revocation is separate.
   - Chat pulley menu: new chat, change model and change provider. Provider switching starts
     a new chat; previous chats stay with their original provider and are not transmitted
     to the new provider. Display the active provider and model.
   - Keep API keys out of logs, history, QML settings and plaintext configuration. Masking
     is not secure storage. Never claim credential persistence works before verifying it.
5. Cover empty, loading, error, cancellation, retry and successful states. Check keyboard
   overlap, long model names/messages, orientation, theme contrast and intentional scrolling.
   Treat model output as untrusted text; do not execute or automatically follow embedded links.
6. Run the smallest available test/build covering the change. A build checks compilation,
   not phone interaction. Validate visual/gesture behaviour on an explicitly authorized device
   or compatible emulator; report unavailable checks rather than claiming screenshots or success.
7. Summarize changes, validation evidence and remaining limitations. Do not expand scope into
   deployment or publishing just because the upstream skill contains those workflows.

## Primary references

- [Official Sailfish documentation](https://docs.sailfishos.org/)
- [Silica API reference](https://sailfishos.org/develop/docs/silica/)
- [Sailjail permissions](https://github.com/sailfishos/sailjail-permissions)
- [Upstream repository at imported revision](https://github.com/ilpianista/sailfishos-skills/tree/fbecb1f558b19d43e28133ec93e571ce3881d0c9)
