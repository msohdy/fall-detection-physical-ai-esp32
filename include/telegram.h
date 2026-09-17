#ifndef TELEGRAM_H
#define TELEGRAM_H

// Starts the WiFi connection and spawns a background FreeRTOS task that
// owns all Telegram sending. Non-blocking -- WiFi.begin() connects in
// the background, and the actual HTTPS send (which can block for
// seconds on a real network call) never runs on the main loop's task,
// so it can never stall IMU sampling, classification, or the
// buzzer/Neopixel animations. (Found during testing: with the send
// called directly from the main loop, a slow/retrying send made the
// whole state machine look "stuck" -- nothing else could run while it
// was in flight.)
void telegramInit();

// Call frequently from the main loop (e.g. every ~20ms tick). Non-
// blocking -- if WiFi has dropped, kicks off a fresh WiFi.begin() at
// most once per RECONNECT_INTERVAL_MS. Passive AutoReconnect alone was
// observed to leave the link disconnected indefinitely after some
// drops, so this actively re-associates instead of just waiting for
// the driver to recover on its own. (WiFi.begin() itself is cheap and
// non-blocking, so this is fine to call from the main loop.)
void telegramTick();

// Fire-and-forget: marks a fall alert as pending. The background task
// keeps retrying until it actually succeeds, independent of whatever
// the state machine does afterward (e.g. if the person recovers before
// WiFi comes up, the alert still goes out once it does).
void telegramSendFallAlert();

// Fire-and-forget: marks the "resolved" message as pending, same
// retry-until-success semantics as telegramSendFallAlert().
void telegramSendResolved();

#endif  // TELEGRAM_H
