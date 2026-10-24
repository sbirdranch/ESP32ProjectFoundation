#pragma once
#include <stdint.h>
#include <stddef.h>
namespace Window {
enum class State { Unknown, Opening, Closing, Open, Closed };
enum class Direction { None, Open, Close };
class Controller {
public:
    Controller(uint32_t runMs = 30000, uint32_t pauseMs = 500)
        : runMs_(runMs), pauseMs_(pauseMs) {}
    bool command(const uint8_t* payload, size_t length, uint32_t now);
    void move(Direction direction, uint32_t now);
    void stop(uint32_t now);
    void update(uint32_t now);
    State state() const { return state_; }
    State endpoint() const { return endpoint_; }
    bool openOutput() const { return active_ == Direction::Open; }
    bool closeOutput() const { return active_ == Direction::Close; }
    const char* stateText() const;
private:
    uint32_t runMs_, pauseMs_, startedAt_ = 0, offAt_ = 0;
    Direction active_ = Direction::None, pending_ = Direction::None;
    Direction lastDirection_ = Direction::None;
    State state_ = State::Unknown, endpoint_ = State::Unknown;
};
}
