# Channels

[![Build status](https://ci.appveyor.com/api/projects/status/10jpolule81p18m6?svg=true)](https://ci.appveyor.com/project/rikdev/channels)
[![codecov](https://codecov.io/gh/rikdev/channels/branch/master/graph/badge.svg)](https://codecov.io/gh/rikdev/channels)

The `Channels` library is an extended implementation of a managed signals/slots
system and is used for communication between threads.
The library includes types like `channel`, `buffered_channel`,
`aggregating_channel` and helpers for working with them.

The class `channel` implements a traditional signals/slots system
(like Qt signals) where some set of callbacks are connected to the channel and
callbacks are called when the transmitter sent the value to the `channel`.

The class `buffered_channel` is similar to `channel` but it stores the last
value sent.
Unlike `std::future`, `buffered_channel` allows to pass more than one value.

The class `aggregating_channel` is similar to `channel` but it passes returned
values from callbacks back to the transmitter (like Boost.Signals2).

If your program uses executors, this library can be easily integrated with them.
