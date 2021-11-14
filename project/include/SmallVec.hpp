//
// Created by tesserakt on 11.11.2021.
//

#pragma once

#include <algorithm>
#include <cassert>

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

  constexpr iterator ptr() {
    if (onHeap())
      return impl.heap;
    return impl.stack;
  }

  constexpr const_iterator ptr() const {
    if (onHeap())
      return impl.heap;
    return impl.stack;
  }

public:
  static constexpr auto size_type = N;

  SmallVec(const SmallVec &rhs) : cap(N), len(0) {
    extendCopying(rhs.begin(), rhs.end());
  }

  SmallVec(SmallVec &&rhs) noexcept : cap(N), len(0) {
    extendConsuming(rhs.begin(), rhs.end());
  }

  ~SmallVec() {
    if (onHeap())
      free(impl.heap);
  }

  explicit SmallVec() noexcept
      : impl({.stack = {}}),
        cap(N),
        len(0) {}

  template <std::size_t M>
  SmallVec(T(&&data)[M])
      : len(M),
        impl() {
    if constexpr (M <= N) {
      cap = N;
      std::swap_ranges(data, data + M, ptr());
    } else {
      T *memory = reinterpret_cast<T *>(malloc(M * sizeof(T)));
      if (memory == nullptr)
        throw std::runtime_error("Failed to allocate memory in SmallVec");
      std::swap_ranges(data, data + M, memory);
      impl.heap = memory;
      cap = M;
    }
  }

  template <typename... U>
  SmallVec(U... tail) noexcept
      : SmallVec({tail...}) {}

  [[nodiscard]] bool onHeap() const { return cap > N; }
  [[nodiscard]] bool onStack() const { return !onHeap(); }

  void grow(std::size_t newSize) {
    assert(newSize >= len);
    if (newSize <= N) {
      // on stack memory already allocated
      if (onStack())
        return;
      // on heap, copy to stack
      auto heapPrt = impl.heap;
      std::swap_ranges(heapPrt, heapPrt + len, std::begin(impl.stack));
      free(heapPrt);
    } else if (newSize != cap) {
      T *memory;
      if (onStack()) {
        // on stack, copy to heap
        memory = reinterpret_cast<T *>(malloc(newSize * sizeof(T)));
        if (memory == nullptr)
          throw std::runtime_error("Failed to allocate memory in SmallVec");
        std::swap_ranges(impl.stack, impl.stack + N, memory);
      } else {
        // on heap, just reallocate
        memory = reinterpret_cast<T *>(realloc(impl.heap, newSize * sizeof(T)));
        if (memory == nullptr) {
          free(impl.heap);
          throw std::runtime_error("Failed to reallocate memory in SmallVec");
        }
      }
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
      free(heapPtr);
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
    ptr()[len] = std::forward<U>(value);
    len++;
  }

  template <typename Iter>
  void extendConsuming(Iter begin, Iter end) {
    auto size = std::distance(begin, end);
    reserveExact(size);
    std::swap_ranges(begin, end, ptr() + len);
    len += size;
  }

  template <typename Iter>
  void extendCopying(Iter begin, Iter end) {
    auto size = std::distance(begin, end);
    reserveExact(size);
    std::copy(begin, end, ptr() + len);
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
    return ptr()[index];
  }

  const T &operator[](std::size_t index) const {
    return ptr()[index];
  }

  iterator begin() {
    return ptr();
  }

  const_iterator begin() const {
    return ptr();
  }

  iterator end() {
    return ptr() + len;
  }

  const_iterator end() const {
    return ptr() + len;
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

template <typename T, typename... U>
SmallVec(T, U...) -> SmallVec<T, 1 + sizeof...(U)>;

}// namespace smallvec