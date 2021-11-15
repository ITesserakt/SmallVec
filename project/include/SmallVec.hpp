//
// Created by tesserakt on 11.11.2021.
//

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <exception>
#include <vector>

namespace smallvec {
template <typename T, std::size_t N>
class SmallVec {
public:
  using iterator = T *;
  using const_iterator = const T *;

private:
  union SmallVecData {
    T stack[N];
    T *heap;
  } impl;
  std::size_t cap;
  std::size_t len;

public:
  static constexpr auto inlineCapacity = N;

  SmallVec(const SmallVec &rhs) : cap(N), len(0), impl() {
    extendCopying(rhs.begin(), rhs.end());
  }

  SmallVec(SmallVec &&rhs) noexcept : cap(N), len(0), impl() {
    extendConsuming(rhs.begin(), rhs.end());
  }

  ~SmallVec() {
    if (onHeap())
      delete[] impl.heap;
  }

  explicit SmallVec() noexcept
      : impl(),
        cap(N),
        len(0) {}

  template <std::size_t M>
  SmallVec(T(&&data)[M])
      : cap(std::max(M, N)),
        len(0),
        impl() {
    extendConsuming(data, data + M);
  }

  template <std::size_t M>
  SmallVec(T (&data)[M])
      : cap(std::max(M, N)),
        len(0),
        impl() {
    extendCopying(data, data + M);
  }

  template <typename... U>
  SmallVec(U... tail) noexcept
      : SmallVec({tail...}) {}

  [[nodiscard]] bool onHeap() const { return cap > N; }
  [[nodiscard]] bool onStack() const { return !onHeap(); }

  constexpr iterator data() {
    if (onHeap())
      return impl.heap;
    return impl.stack;
  }

  constexpr const_iterator data() const {
    if (onHeap())
      return impl.heap;
    return impl.stack;
  }

  void grow(std::size_t newSize) {
    assert(newSize >= len);
    if (newSize <= N) {
      // on stack memory already allocated
      if (onStack())
        return;
      // on heap, copy to stack
      auto heapPrt = impl.heap;
      std::swap_ranges(heapPrt, heapPrt + len, std::begin(impl.stack));
      delete[] heapPrt;
    } else if (newSize != cap) {
      T *memory = new T[newSize];
      std::swap_ranges(data(), data() + len, memory);
      if (onHeap())
        delete[] impl.heap;
      impl.heap = memory;
    }
    cap = newSize;
  }

  void reserve(std::size_t additional) {
    if (cap - len >= additional)
      return;
    constexpr auto nextPowerOfTwo = [](std::size_t n) {
      std::size_t result = 1;
      while (n > result)
        result <<= 1;
      return result;
    };
    auto newSize = nextPowerOfTwo(len + additional);
    grow(newSize);
  }

  void reserveExact(std::size_t additional) {
    if (cap - len >= additional)
      return;
    auto newSize = len + additional;
    grow(newSize);
  }

  void shrink() {
    if (onStack())
      return;
    if (N >= len) {
      auto heapPtr = impl.heap;
      std::swap_ranges(heapPtr, heapPtr + len, impl.stack);
      delete[] heapPtr;
      cap = len;
    } else if (cap > len) {
      grow(len);
    }
  }

  [[nodiscard]] std::size_t size() const { return len; }

  [[nodiscard]] std::size_t capacity() const { return cap; }

  [[nodiscard]] std::size_t takenSize() const {
    if (onHeap())
      return sizeof(SmallVec) + (cap + 1) * sizeof(T);
    return sizeof(SmallVec);
  }

  template <typename U>
  void push(U &&value) {
    if (len == cap)
      reserve(1);
    data()[len] = std::forward<U>(value);
    len++;
  }

  template <typename Iter>
  void extendConsuming(Iter begin, Iter end) {
    auto size = std::distance(begin, end);
    reserveExact(size);
    std::swap_ranges(begin, end, data() + len);
    len += size;
  }

  template <typename Iter>
  void extendCopying(Iter begin, Iter end) {
    auto size = std::distance(begin, end);
    reserveExact(size);
    std::copy(begin, end, data() + len);
    len += size;
  }

  std::vector<T> intoVector() && {
    std::vector<T> result(len);
    if (onStack())
      std::swap_ranges(begin(), end(), result.begin());
    else
      result = {begin(), end()};
    return result;
  }

  void pop() {
    if (len > 0)
      len--;
  }

  T &back() {
    return this[len - 1];
  }

  const T &back() const {
    return this[len - 1];
  }

  T &operator[](std::size_t index) {
    return data()[index];
  }

  const T &operator[](std::size_t index) const {
    return data()[index];
  }

  iterator begin() {
    return data();
  }

  const_iterator begin() const {
    return data();
  }

  iterator end() {
    return data() + len;
  }

  const_iterator end() const {
    return data() + len;
  }
};

template <std::size_t N, typename T, template <typename> typename Container>
auto fromContainer(Container<T> &&data) {
  SmallVec<T, N> result;
  result.extendConsuming(std::begin(data), std::end(data));
  return result;
}

template <std::size_t N, typename T>
auto fromContainer(T *data, std::size_t n) {
  SmallVec<T, N> result;
  result.extendConsuming(data, data + n);
  return result;
}

template <typename T, std::size_t M, std::size_t N = M>
auto fromContainer(std::array<T, M> data) {
  SmallVec<T, N> result;
  result.extendConsuming(data.begin(), data.end());
  return result;
}

template <typename T, std::size_t N>
SmallVec(T(&&)[N]) -> SmallVec<T, N>;

template <typename T, std::size_t N>
SmallVec(T (&)[N]) -> SmallVec<T, N>;

template <typename T, typename... U>
SmallVec(T, U...) -> SmallVec<T, 1 + sizeof...(U)>;
}// namespace smallvec