#include <JC_Button.h>

Button button(BUTTON_PIN);

void buttonLoop() {
    button.read();

    if (button.pressedFor(LONG_PRESS)) {
        should_save_config = false;
        configPortal(false);
    }
}

void buttonSetup() {
    button.begin();
}
