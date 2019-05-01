void buttonLoop() {
    button.read();

    if (button.pressedFor(LONG_PRESS)) {
        startConfigPortal();
    }
}

void buttonSetup() {
    button.begin();
}
