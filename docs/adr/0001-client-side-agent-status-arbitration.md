# Client-Side Agent Status Arbitration

Accepted.

Clawd Mochi resolves multi-client work status on the computer, not inside the ESP32 firmware. A Unified Agent Watcher observes Codex and Claude Code, applies the Web-controlled Display Mode, and sends one final status update to the device; the device stores only the Display Mode and current display result. This keeps the firmware small and avoids storing parallel client state on the device, at the cost of requiring the unified watcher for supported multi-client use.
