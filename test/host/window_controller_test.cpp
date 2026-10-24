#include <cassert>
#include <cstdio>
#include <cstring>
#include "WindowOutputs.h"
unsigned long testMillis = 0;
bool pins[50] = {};
void digitalWrite(int pin, int value) { pins[pin] = value; assert(!(pins[10] && pins[11])); }
void pinMode(int, int) {}
using namespace Window;
void check(uint32_t start) {
    Controller c;
    WindowOutputs outputs(10, 11);
    outputs.begin();
    assert(c.state() == State::Unknown);
    auto tick = [&](uint32_t elapsed) { c.update(start + elapsed); outputs.apply(c); };
    c.move(Direction::Open, start); outputs.apply(c);
    tick(29999); assert(c.openOutput());
    c.move(Direction::Open, start + 29999); tick(30000); assert(c.openOutput());
    tick(59999); assert(c.state() == State::Open && !c.openOutput());
    c.move(Direction::Close, start + 60000); outputs.apply(c); assert(!c.closeOutput());
    tick(60498); assert(!c.closeOutput());
    tick(60499); assert(c.closeOutput());
    assert(c.endpoint() == State::Open);
    c.move(Direction::Open, start + 60500); outputs.apply(c);
    tick(60999); assert(!c.openOutput());
    tick(61000); assert(c.openOutput());
    c.move(Direction::Close, start + 61001); outputs.apply(c);
    c.stop(start + 61002); outputs.apply(c);
    tick(62000); assert(!c.openOutput() && !c.closeOutput() && c.state() == State::Unknown);
    c.move(Direction::Close, start + 63000); tick(93000);
    assert(c.state() == State::Closed);
    const auto invalid = reinterpret_cast<const uint8_t*>("OPEN ");
    assert(!c.command(invalid, 5, start + 94000)); assert(c.state() == State::Closed);
    c.move(Direction::Open, start + 94000); outputs.apply(c);
    c.stop(start + 94001); outputs.apply(c); // OTA cancellation of active output.
    tick(150000); assert(c.state() == State::Unknown && !c.openOutput());
    c.move(Direction::Open, start + 160000); outputs.apply(c);
    c.move(Direction::Close, start + 160001); outputs.apply(c);
    c.move(Direction::Close, start + 160200); outputs.apply(c);
    tick(160500); assert(!c.closeOutput());
    tick(160501); assert(c.closeOutput());
    tick(190500); assert(c.closeOutput());
    tick(190501); assert(c.state() == State::Closed && !c.closeOutput());
    c.move(Direction::Open, start + 200000); outputs.apply(c);
    c.move(Direction::Close, start + 200001); outputs.apply(c);
    c.move(Direction::Open, start + 200002); outputs.apply(c);
    assert(c.openOutput() && !c.closeOutput()); // Cancel reversal back to original direction.
    c.stop(start + 200003); outputs.apply(c);
}
int main() { check(0); check(UINT32_MAX - 1000); puts("Window controller/output tests passed"); }
