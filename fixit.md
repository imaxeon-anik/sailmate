# Sailmate fix-it list

- [x] **Delete chats:** Long-press a chat thread and confirm Delete chat to remove its local messages and saved photos. Other chats, profiles and keys are retained.
- [x] **Last-used time:** Show the last saved message activity time on the right side of each history row, using a smaller font: hh:mm for today, d MMM earlier this year, MMM yyyy for older chats. Opening a chat does not update this time.
- [x] **Help version placement:** Display the app version number immediately below the Help heading.
- [x] **Chat scrolling:** Opening a chat shows the latest message. Streaming replies update in place, so the chat can be scrolled while a response arrives; it follows new text only while scrolled to the bottom.
- [x] **Push-up menu in chat:** Pull up at the end of a chat for Change model and Home, so long chats do not need scrolling back to the top. Start new chat lives only in the chat list.
- [x] **Long replies:** Replies may use up to 4,096 tokens (16,384 for OpenAI reasoning models). A reply that hits the limit is kept, saved and marked "Cut off at the length limit" instead of being discarded.
- [x] **Stream timeout:** The 2-minute timeout now applies only when the provider sends nothing, not to the whole reply.
- [x] **OpenAI-compatible streams:** Accept common variations from compatible servers (missing choice index, empty tool-call lists, end-of-stream without [DONE], eos finish reasons).
- [x] **Custom profiles connect on tap:** A custom profile with a saved key loads it and connects straight away, like OpenAI and Anthropic.
- [x] **Model picker:** Stays open and shows the reason when a model change is refused.
- [x] **Harbour packaging:** Removed the scalable SVG icon and the SQLite plugin requirement again.

Native build and device validation are pending; the SDK host is currently unavailable.
