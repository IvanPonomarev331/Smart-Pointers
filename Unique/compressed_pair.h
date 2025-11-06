#pragma once

#include <stdio.h>
#include <type_traits>
#include <utility>

template <typename T, size_t Index, bool Empty = std::is_empty_v<T> && !std::is_final_v<T>>
class CompressedElem;

template <typename T, size_t Index>
class CompressedElem<T, Index, true> : private T {
public:
    CompressedElem() : T() {
    }

    template <typename U, typename = std::enable_if_t<std::is_constructible_v<T, U&&>>>
    explicit CompressedElem(U&& value) : T(std::move(value)) {
    }

    T& Get() {
        return *this;
    }
    const T& Get() const {
        return *this;
    }
};

template <typename T, size_t Index>
class CompressedElem<T, Index, false> {
public:
    CompressedElem() : value_() {
    }

    template <typename U, typename = std::enable_if_t<std::is_constructible_v<T, U&&>>>
    explicit CompressedElem(U&& value) : value_(std::move(value)) {
    }

    T& Get() {
        return value_;
    }
    const T& Get() const {
        return value_;
    }

private:
    T value_;
};

template <typename F, typename S>
class CompressedPair : public CompressedElem<F, 0>, public CompressedElem<S, 1> {
    using First = CompressedElem<F, 0>;
    using Second = CompressedElem<S, 1>;

public:
    CompressedPair() = default;

    CompressedPair(const F& first, const S& second) : First(first), Second(second) {
    }

    CompressedPair(F&& first, const S& second) : First(std::move(first)), Second(second) {
    }

    CompressedPair(const F& first, S&& second) : First(first), Second(std::move(second)) {
    }

    CompressedPair(F&& first, S&& second) : First(std::move(first)), Second(std::move(second)) {
    }

    F& GetFirst() {
        return First::Get();
    }
    const F& GetFirst() const {
        return First::Get();
    }

    S& GetSecond() {
        return Second::Get();
    }
    const S& GetSecond() const {
        return Second::Get();
    }
};
