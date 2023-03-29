# A Proposal to add log library

## Motivation

Logging is a crucial aspect of modern software development, as it helps
developers track, analyze, and debug the execution of applications. In current
C++ Standard Library, there is no standadized way of logging which is a common
practice in other langauges. Currently, the C++ standard library does not
include a built-in logging mechanism. As a result, developers must rely on
third-party libraries or create custom logging solutions, which can lead to
inconsistencies and unnecessary overhead.

To address this issue, we propose a new set of simple, yet extensible logging
APIs that for the standard C++ library. The new proposed logging APIs is
designed to be easy to use, efficient, and flexible enough to accommodate the
varying needs of developers, while also adhering to the conventional patterns
observed in existing logging libraries.

## Design Decisions

The design is built upon the opentelemetry-cpp Logs APIs decoupling from the
logging backend which usually defines where the log is directed, such as stderr,
a local file or a network location.

## Proposed Log APIs Design

The logging API will reside in the `std::log` namespace and consist of the
following classes and functions.

```cpp

namespace std::log

{

enum class Severity;

std::shared_ptr<std::logger> default_logger();

void set_default_logger(std::shared_ptr<std::logger> default_logger);

template <typename... ArgumentType>
void log(Severity severity, ArgumentType &&... args) noexcept;

template <typename... ArgumentType>
void trace(ArgumentType &&... args) noexcept;

template <typename... ArgumentType>
void debug(ArgumentType &&... args) noexcept;

template <typename... ArgumentType>
void info(ArgumentType &&... args) noexcept;

template <typename... ArgumentType>
void warn(ArgumentType &&... args) noexcept;

template <typename... ArgumentType>
void error(ArgumentType &&... args) noexcept;

template <typename... ArgumentType>
void fatal(ArgumentType &&... args) noexcept;

void set_severity(Severity severity);

}

```

## Example Usage

```cpp

#include <iostream>
#include <memory>
#include <log>

int main()
{
    // log with default_logger();
    std::log::info("Hello std::log");

    std::log::warn("Hello std::log", {{"key1": 1}, {"key2": 2}});

    // create a new logger.
    std::shared_ptr<logger> new_logger = ...;
    std::log::set_default_logger(new_logger);

    // log to the new logger which defines where the log should be exported.
    std::log::error("Error occurred", {{"error_code", 3}});

    // reset to the default logger.
    std::log::set_default_logger(nullptr);

    return 0;
}

```

## Significant Changes to Reivew

## Wording

## Acknowledgements
https://github.com/open-telemetry/opentelemetry-cpp/graphs/contributors
