#ifndef TELEGRAM_H
#define TELEGRAM_H

// Sends a message to the configured Telegram chat via the Bot API.
// Non-blocking-ish: uses a short HTTP timeout so a WiFi/API failure
// means "message skipped this cycle," never a stalled main loop.
// Safe to call even if WiFi isn't connected -- it just skips.
void sendTelegramMessage(const char* message);

#endif  // TELEGRAM_H
