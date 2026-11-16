#pragma once

#include <optional>
#include <string>
#include <utility>

namespace dayflow {

enum class ErrorCode {
    none,
    invalid_argument,
    not_found,
    database,
    io,
};

struct Error {
    ErrorCode code = ErrorCode::none;
    std::string message;

    Error() = default;
    Error(ErrorCode value, std::string text) : code(value), message(std::move(text)) {}

    explicit operator bool() const noexcept { return code != ErrorCode::none; }
};

inline Error make_error(ErrorCode code, std::string message)
{
    return Error{code, std::move(message)};
}

template <typename T>
class Result {
public:
    Result(T value) : value_(std::move(value)) {}
    Result(Error error) : error_(std::move(error)) {}

    bool ok() const noexcept { return value_.has_value(); }
    explicit operator bool() const noexcept { return ok(); }

    T& value() noexcept { return *value_; }
    const T& value() const noexcept { return *value_; }

    const Error& error() const noexcept { return error_; }

private:
    std::optional<T> value_;
    Error error_;
};

}  // namespace dayflow
