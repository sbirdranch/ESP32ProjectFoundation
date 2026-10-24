#pragma once
#include <Arduino.h>
#include "WindowController.h"
class WindowOutputs {
public:
    WindowOutputs(int openPin, int closePin) : openPin_(openPin), closePin_(closePin) {}
    void begin() {
        digitalWrite(openPin_, LOW); digitalWrite(closePin_, LOW);
        pinMode(openPin_, OUTPUT); pinMode(closePin_, OUTPUT);
    }
    void apply(const Window::Controller& controller) {
        // Remove the inactive direction before asserting the other output.
        if (!controller.openOutput()) digitalWrite(openPin_, LOW);
        if (!controller.closeOutput()) digitalWrite(closePin_, LOW);
        if (controller.openOutput()) digitalWrite(openPin_, HIGH);
        if (controller.closeOutput()) digitalWrite(closePin_, HIGH);
    }
private:
    int openPin_, closePin_;
};
