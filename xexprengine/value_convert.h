#pragma once
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sstream>
#include <complex>


namespace xexprengine {

namespace value_convert {
inline std::string to_string(std::nullptr_t) { return "nullptr"; }
inline std::string to_string(bool value) { return value ? "true" : "false"; }
inline std::string to_string(const std::string &value) { return value; }
inline std::string to_string(const char *value) { return value; }

template <typename T>
inline typename std::enable_if<std::is_arithmetic<T>::value &&
                                   !std::is_same<T, bool>::value,
                               std::string>::type
to_string(T value) {
  return std::to_string(value);
}

template <typename T>
inline std::string to_string(const std::complex<T> &value) {
  std::ostringstream oss;
  oss << "(" << value.real();
  if (value.imag() >= T(0)) {
    oss << "+";
  }
  oss << value.imag() << "i)";
  return oss.str();
}

template <typename K, typename V, typename Compare, typename Alloc>
std::string to_string(const std::map<K, V, Compare, Alloc> &map) {
  if (map.empty())
    return "{}";

  std::string result = "{";
  auto it = map.begin();
  result += to_string(it->first) + ": " + to_string(it->second);

  for (++it; it != map.end(); ++it) {
    result += ", " + to_string(it->first) + ": " + to_string(it->second);
  }

  return result + "}";
}

template <typename K, typename Compare, typename Alloc>
std::string to_string(const std::set<K, Compare, Alloc> &set) {
  if (set.empty())
    return "{}";

  std::string result = "{";
  auto it = set.begin();
  result += to_string(*it);

  for (++it; it != set.end(); ++it) {
    result += ", " + to_string(*it);
  }

  return result + "}";
}

template <typename K, typename V, typename Hash, typename Pred, typename Alloc>
std::string to_string(const std::unordered_map<K, V, Hash, Pred, Alloc> &map) {
  if (map.empty())
    return "{}";

  std::string result = "{";
  auto it = map.begin();
  result += to_string(it->first) + ": " + to_string(it->second);

  for (++it; it != map.end(); ++it) {
    result += ", " + to_string(it->first) + ": " + to_string(it->second);
  }

  return result + "}";
}

template <typename K, typename Hash, typename Pred, typename Alloc>
std::string to_string(const std::unordered_set<K, Hash, Pred, Alloc> &set) {
  if (set.empty())
    return "{}";

  std::string result = "{";
  auto it = set.begin();
  result += to_string(*it);

  for (++it; it != set.end(); ++it) {
    result += ", " + to_string(*it);
  }

  return result + "}";
}

template <typename T, typename Alloc>
std::string to_string(const std::vector<T, Alloc> &vec) {
  if (vec.empty())
    return "[]";

  std::string result = "[";
  for (size_t i = 0; i < vec.size(); ++i) {
    if (i > 0)
      result += ", ";
    result += to_string(vec[i]);
  }
  return result + "]";
}

template <typename T, typename Alloc>
std::string to_string(const std::list<T, Alloc> &list) {
  if (list.empty())
    return "[]";

  std::string result = "[";
  auto it = list.begin();
  result += to_string(*it);

  for (++it; it != list.end(); ++it) {
    result += ", " + to_string(*it);
  }
  return result + "]";
}

template <typename T> inline std::string to_string(const T &value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}
} // namespace value_convert
} // namespace xexprengine