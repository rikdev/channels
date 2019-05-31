#pragma once

namespace channels {

class connection;

// channels

template<typename... Ts>
class channel;

template<typename... Ts>
class buffered_channel;

template<typename F>
class aggregating_channel;

// transmitters

template<typename Channel>
class transmitter;

// exceptions

class callbacks_exception;
struct channel_error;
struct transmitter_error;

} // namespace channels
