#include "WindowController.h"
#include <string.h>
namespace Window {
bool Controller::command(const uint8_t* payload, size_t length, uint32_t now) {
    if (length == 4 && memcmp(payload, "OPEN", 4) == 0) move(Direction::Open, now);
    else if (length == 5 && memcmp(payload, "CLOSE", 5) == 0) move(Direction::Close, now);
    else if (length == 4 && memcmp(payload, "STOP", 4) == 0) stop(now);
    else return false;
    return true;
}
void Controller::move(Direction direction, uint32_t now) {
    if (direction == Direction::None) { stop(now); return; }
    if (active_ != Direction::None && active_ != direction) {
        lastDirection_ = active_;
        active_ = Direction::None;
        offAt_ = now;
    }
    pending_ = direction;
    state_ = direction == Direction::Open ? State::Opening : State::Closing;
    if (active_ == direction || lastDirection_ == Direction::None ||
        lastDirection_ == direction || uint32_t(now - offAt_) >= pauseMs_) {
        active_ = direction;
        lastDirection_ = direction;
        pending_ = Direction::None;
        startedAt_ = now;
    }
}
void Controller::stop(uint32_t now) {
    if (active_ != Direction::None) { lastDirection_ = active_; offAt_ = now; }
    active_ = pending_ = Direction::None;
    state_ = endpoint_ = State::Unknown;
}
void Controller::update(uint32_t now) {
    if (pending_ != Direction::None && uint32_t(now - offAt_) >= pauseMs_) {
        active_ = lastDirection_ = pending_;
        pending_ = Direction::None;
        startedAt_ = now;
    }
    if (active_ != Direction::None && uint32_t(now - startedAt_) >= runMs_) {
        state_ = endpoint_ = active_ == Direction::Open ? State::Open : State::Closed;
        lastDirection_ = active_;
        active_ = Direction::None;
        offAt_ = now;
    }
}
const char* Controller::stateText() const {
    switch (state_) {
        case State::Opening: return "Opening";
        case State::Closing: return "Closing";
        case State::Open: return "Open";
        case State::Closed: return "Closed";
        default: return "Unknown";
    }
}
}
