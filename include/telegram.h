#ifndef TELEGRAM_H
#define TELEGRAM_H

// Starts the WiFi connection. Non-blocking -- WiFi.begin() connects in
// the background; nothing here waits for it to complete.
void telegramInit();

// Sends a message to the configured Telegram chat via the Bot API.
// Returns true only if it was actually sent (WiFi connected and the
// HTTP request succeeded). Never blocks waiting for WiFi to connect --
// if it's not already connected, returns false immediately. Per the
// PRD: a WiFi failure means "skip this cycle," not a stalled main
// loop -- callers should treat false as retryable, not fatal.
bool sendTelegramMessage(const char* message);

#endif  // TELEGRAM_H
