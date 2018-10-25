#include <JC_Button.h>

Button button(BUTTON_PIN);

void buttonLoop() {
    button.read();

    if (button.pressedFor(LONG_PRESS)) {
        should_save_config = false;
        configPortal(false);
        delay(2000);
        ESP.restart();
        delay(5000);
    }
}

void buttonSetup() {
    button.begin();
}
