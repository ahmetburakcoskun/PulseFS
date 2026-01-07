#pragma once

#include <optional>
#include <string>
#include <variant>

namespace PulseFS::Utils {

template <typename T, typename E = std::string> class Result {
public:
  Result(T value) : m_data(std::move(value)) {}
  Result(E error) : m_data(std::move(error)) {}

  [[nodiscard]] bool HasValue() const noexcept {
    return std::holds_alternative<T>(m_data);
  }

  [[nodiscard]] bool HasError() const noexcept {
    return std::holds_alternative<E>(m_data);
  }

  [[nodiscard]] explicit operator bool() const noexcept { return HasValue(); }

  [[nodiscard]] T &Value() & { return std::get<T>(m_data); }

  [[nodiscard]] const T &Value() const & { return std::get<T>(m_data); }

  [[nodiscard]] T &&Value() && { return std::get<T>(std::move(m_data)); }

  [[nodiscard]] const E &Error() const & { return std::get<E>(m_data); }

  [[nodiscard]] E &&Error() && { return std::get<E>(std::move(m_data)); }

private:
  std::variant<T, E> m_data;
};

template <typename E> class Result<void, E> {
public:
  Result() : m_error() {}
  Result(E error) : m_error(std::move(error)) {}

  [[nodiscard]] bool HasValue() const noexcept { return !m_error.has_value(); }

  [[nodiscard]] bool HasError() const noexcept { return m_error.has_value(); }

  [[nodiscard]] explicit operator bool() const noexcept { return HasValue(); }

  [[nodiscard]] const E &Error() const & { return m_error.value(); }

  [[nodiscard]] E &&Error() && { return std::move(m_error.value()); }

private:
  std::optional<E> m_error;
};

template <typename E> Result<void, E> Ok() { return Result<void, E>(); }

template <typename E> Result<void, E> Err(E error) {
  return Result<void, E>(std::move(error));
}

} // namespace PulseFS::Utils
