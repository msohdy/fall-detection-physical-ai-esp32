#include "state_machine.h"
#include "model_inference.h"
#include "neopixel_driver.h"
#include "buzzer_driver.h"
#include "telegram.h"
#include <string.h>

namespace {

enum class AppState {
  Normal,
  Dizzy,
  Alarmed,
  Recovering,
};

AppState state = AppState::Normal;
int confirmCounter = 0;
bool alertSent = false;

// Sustained-reading thresholds, counted in classifications (one per
// ~1.5s window -- see main.cpp's non-overlapping-window classification
// rate) -- not single-frame triggers, so a lone misread can never
// silence a real alarm early or falsely confirm recovery.
const int CONFIRM_THRESHOLD = 3;           // ~4.5s sustained before ALARMED -> RECOVERING
const int RECOVER_CONFIRM_THRESHOLD = 5;   // ~7.5s sustained before RECOVERING -> NORMAL

bool isLabel(int classIndex, const char* name) {
  return strcmp(modelLabel(classIndex), name) == 0;
}
bool isFall(int c) { return isLabel(c, "fall"); }
bool isDizzy(int c) { return isLabel(c, "dizzy"); }
bool isNormalMotion(int c) { return isLabel(c, "stand") || isLabel(c, "sit") || isLabel(c, "walk"); }

// What counts as "recovering" right after a fall (exiting ALARMED),
// per an explicit choice to require stand/dizzy/walk specifically --
// NOT sit. Any sign of movement/consciousness other than sitting
// still counts; a continued "fall" or "sit" reading keeps the siren
// going. Deliberately narrower than isNormalMotion() above, which is
// still used for Dizzy -> Normal and Recovering -> Normal.
bool isRecoverySign(int c) { return isLabel(c, "stand") || isDizzy(c) || isLabel(c, "walk"); }

// Standing/Sitting/Walking are steady (non-animated) colors, so
// calling this every tick while remaining in NORMAL is harmless --
// unlike Alarmed/Recovering, there's no animation phase to disrupt.
void updateNeopixelForNormal(int c) {
  if (isLabel(c, "stand")) neopixelSetState(VisualState::Standing);
  else if (isLabel(c, "sit")) neopixelSetState(VisualState::Sitting);
  else if (isLabel(c, "walk")) neopixelSetState(VisualState::Walking);
}

void enterAlarmed() {
  state = AppState::Alarmed;
  neopixelSetState(VisualState::Alarmed);
  buzzerSetState(BuzzerState::Siren);
  alertSent = false;
  confirmCounter = 0;
}

}  // namespace

void stateMachineInit() {
  state = AppState::Normal;
  confirmCounter = 0;
  alertSent = false;
  neopixelSetState(VisualState::Standing);
  buzzerSetState(BuzzerState::Silent);
}

void stateMachineUpdate(int predicted) {
  switch (state) {
    case AppState::Normal:
      updateNeopixelForNormal(predicted);
      if (isFall(predicted)) {
        enterAlarmed();
      } else if (isDizzy(predicted)) {
        state = AppState::Dizzy;
        neopixelSetState(VisualState::Dizzy);
        buzzerSetState(BuzzerState::Warning);
      }
      break;

    case AppState::Dizzy:
      if (isFall(predicted)) {
        enterAlarmed();
      } else if (isNormalMotion(predicted)) {
        state = AppState::Normal;
        updateNeopixelForNormal(predicted);
        buzzerSetState(BuzzerState::Silent);
      }
      break;

    case AppState::Alarmed:
      if (!alertSent) {
        sendTelegramMessage("Fall detected. Siren active.");
        alertSent = true;  // fire once per event, never resend while ALARMED
      }
      if (isRecoverySign(predicted)) {
        confirmCounter++;
        if (confirmCounter >= CONFIRM_THRESHOLD) {
          state = AppState::Recovering;
          neopixelSetState(VisualState::Recovering);
          buzzerSetState(BuzzerState::Recovery);
          confirmCounter = 0;
        }
      } else {
        confirmCounter = 0;  // any wobble (including "sit") resets -- must be a clean, sustained recovery
      }
      break;

    case AppState::Recovering:
      if (isNormalMotion(predicted)) {
        confirmCounter++;
        if (confirmCounter >= RECOVER_CONFIRM_THRESHOLD) {
          state = AppState::Normal;
          confirmCounter = 0;
          updateNeopixelForNormal(predicted);
          buzzerSetState(BuzzerState::Silent);
          sendTelegramMessage("Resolved: back to normal activity.");
        }
      } else if (isFall(predicted)) {
        enterAlarmed();  // relapse -- resets alertSent so a fresh alert goes out
      } else {
        confirmCounter = 0;
      }
      break;
  }
}
