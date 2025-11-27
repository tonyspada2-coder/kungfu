// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2019-05-25.
//

#ifndef KUNGFU_COMMON_H
#define KUNGFU_COMMON_H

#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

//------------------------------------------------------------------------
// pack struct for fixing data length in journal
#ifdef _WINDOWS
#define strcpy(dest, src) strcpy_s(dest, sizeof(dest), src)
#define strncpy(dest, src, max_length) strncpy_s(dest, sizeof(dest), src, max_length)
#define KF_PACK_TYPE_BEGIN __pragma(pack(push, 1))
#define KF_PACK_TYPE_END                                                                                               \
  ;                                                                                                                    \
  __pragma(pack(pop))
#else
#define KF_PACK_TYPE_BEGIN
#define KF_PACK_TYPE_END __attribute__((packed));
#endif
//------------------------------------------------------------------------

#define DECLARE_PTR(X) typedef std::shared_ptr<X> X##_ptr; /** define smart ptr */
#define FORWARD_DECLARE_CLASS_PTR(X)                                                                                   \
  class X;                                                                                                             \
  DECLARE_PTR(X) /** forward defile smart ptr */
#define FORWARD_DECLARE_STRUCT_PTR(X)                                                                                  \
  struct X;                                                                                                            \
  DECLARE_PTR(X) /** forward defile smart ptr */

namespace kungfu {
uint32_t hash_32(const unsigned char *key, int32_t length);

template <typename V, size_t N, typename = void> struct array_to_string;

template <typename V, size_t N> struct array_to_string<V, N, std::enable_if_t<std::is_same_v<V, char>>> {
  std::string operator()(const V *v) { return std::string(v); };
};

template <typename V, size_t N> struct array_to_string<V, N, std::enable_if_t<not std::is_same_v<V, char>>> {
  std::string operator()(const V *v) {
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < N; i++) {
      ss << (i > 0 ? "," : "") << v[i];
    }
    ss << "]";
    return ss.str();
  };
};

KF_PACK_TYPE_BEGIN
template <typename T, size_t N> struct array {
  static constexpr size_t length = N;
  using element_type = T;
  using type = T[N];
  type value;

  array() {
    if constexpr (std::is_same_v<T, char>) {
      memset(value, '\0', sizeof(value));
    } else {
      memset(value, 0, sizeof(value));
    }
  }

  explicit array(const T *t) { memcpy(value, t, sizeof(value)); }

  explicit array(const unsigned char *t) { memcpy(value, t, sizeof(value)); }

  [[nodiscard]] size_t size() const { return N; }

  [[nodiscard]] std::string to_string() const { return array_to_string<T, N>{}(value); }

  operator T *() { return static_cast<T *>(value); }

  operator const T *() const { return static_cast<const T *>(value); }

  operator const void *() const { return static_cast<const void *>(value); }

  operator std::string() const { return to_string(); }

  T &operator[](int i) const { return const_cast<T &>(value[i]); }

  array<T, N> &operator=(const T *data) {
    if (value == data) {
      return *this;
    }
    if constexpr (std::is_same_v<T, char>) {
      memcpy(value, data, strlen(data));
    } else {
      memcpy(value, data, sizeof(value));
    }
    return *this;
  }

  array<T, N> &operator=(const array<T, N> &other) { return operator=(other.value); }
} KF_PACK_TYPE_END;

template <typename T, size_t N> [[maybe_unused]] void to_json(nlohmann::json &j, const array<T, N> &value) {
  j = value.value;
}

template <typename T, size_t N> [[maybe_unused]] void from_json(const nlohmann::json &j, array<T, N> &value) {
  for (int i = 0; i < N; i++) {
    value.value[i] = j[i].get<T>();
  }
}

template <typename T, size_t N> std::ostream &operator<<(std::ostream &os, array<T, N> t) {
  return os << t.to_string();
}

struct event {
  virtual ~event() = default;

  [[nodiscard]] virtual int64_t gen_time() const = 0;

  [[nodiscard]] virtual int64_t trigger_time() const = 0;

  [[nodiscard]] virtual int32_t msg_type() const = 0;

  [[nodiscard]] virtual uint32_t source() const = 0;

  [[nodiscard]] virtual uint32_t dest() const = 0;

  [[nodiscard]] virtual uint32_t data_length() const = 0;

  [[nodiscard]] virtual const void *data_address() const = 0;

  [[nodiscard]] virtual const char *data_as_bytes() const = 0;

  [[nodiscard]] virtual std::string data_as_string() const = 0;

  [[nodiscard]] virtual std::string to_string() const = 0;

  template <typename T> const T &data() const {
    return *(reinterpret_cast<const T *>(data_address()));
  }
};

DECLARE_PTR(event)
} // namespace kungfu
#endif // KUNGFU_COMMON_H
