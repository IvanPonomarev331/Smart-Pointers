#pragma once

#include "compressed_pair.h"
#include "deleters.h"

#include <algorithm>
#include <cstddef>  // std::nullptr_t
#include <type_traits>

template <typename T>
struct Slug {
    Slug() = default;

    template <typename U>
    Slug(const Slug<U>&) noexcept {
    }

    template <typename U>
    Slug(Slug<U>&&) noexcept {
    }

    template <typename U>
    Slug& operator=(const Slug<U>&) noexcept {
        return *this;
    }

    template <typename U>
    Slug& operator=(Slug<U>&&) noexcept {
        return *this;
    }

    void operator()(T* ptr) const noexcept {
        delete ptr;
    }
};

template <typename T>
struct Slug<T[]> {
    void operator()(T* ptr) const noexcept {
        delete[] ptr;
    }
};

// Primary template
template <typename T, typename Deleter = Slug<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) {
        object_.GetFirst() = ptr;
        object_.GetSecond() = Deleter();
    }
    UniquePtr(T* ptr, Deleter deleter) {
        object_.GetFirst() = ptr;
        object_.GetSecond() = std::move(deleter);
    }

    UniquePtr(const UniquePtr& other) = delete;
    UniquePtr& operator=(const UniquePtr& other) = delete;

    template <typename U, typename E>
    UniquePtr(UniquePtr<U, E>&& other) noexcept {
        object_.GetFirst() = other.Release();
        object_.GetSecond() = std::move(other.GetDeleter());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    template <typename U, typename E>
    UniquePtr& operator=(UniquePtr<U, E>&& other) noexcept {
        if (reinterpret_cast<const void*>(object_.GetFirst()) ==
            reinterpret_cast<const void*>(other.Get())) {
            return *this;
        }
        object_.GetSecond()(object_.GetFirst());
        object_.GetFirst() = other.Release();
        object_.GetSecond() = std::move(other.GetDeleter());
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        object_.GetSecond()(object_.GetFirst());
        object_.GetFirst() = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        object_.GetSecond()(object_.GetFirst());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* obj = object_.GetFirst();
        object_.GetFirst() = nullptr;
        return obj;
    }
    void Reset(T* ptr = nullptr) {
        std::swap(object_.GetFirst(), ptr);
        object_.GetSecond()(ptr);
    }
    void Swap(UniquePtr& other) {
        std::swap(object_.GetFirst(), other.object_.GetFirst());
        std::swap(object_.GetSecond(), other.object_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return object_.GetFirst();
    }
    Deleter& GetDeleter() {
        return object_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return object_.GetSecond();
    }
    explicit operator bool() const {
        return object_.GetFirst();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    template <typename U = T, typename = std::enable_if_t<!std::is_void<U>::value>>
    U& operator*() const {
        return *object_.GetFirst();
    }
    template <typename U = T, typename = std::enable_if_t<!std::is_void<U>::value>>
    U* operator->() const {
        return object_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> object_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) noexcept {
        object_.GetFirst() = ptr;
        object_.GetSecond() = Deleter();
    }
    UniquePtr(T* ptr, Deleter deleter) noexcept {
        object_.GetFirst() = ptr;
        object_.GetSecond() = std::move(deleter);
    }

    template <typename U, typename E>
    UniquePtr(UniquePtr<U, E>&& other) noexcept {
        object_.GetFirst() = other.Release();
        object_.GetSecond() = std::move(other.object_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    template <typename U, typename E>
    UniquePtr& operator=(UniquePtr<U, E>&& other) noexcept {
        if (reinterpret_cast<const void*>(object_.GetFirst()) ==
            reinterpret_cast<const void*>(other.Get())) {
            return *this;
        }
        object_.GetSecond()(object_.GetFirst());
        object_.GetFirst() = other.Release();
        object_.GetSecond() = std::move(other.object_.GetSecond());
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) noexcept {
        object_.GetSecond()(object_.GetFirst());
        object_.GetFirst() = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        object_.GetSecond()(object_.GetFirst());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* obj = object_.GetFirst();
        object_.GetFirst() = nullptr;
        return obj;
    }
    void Reset(T* ptr = nullptr) {
        std::swap(object_.GetFirst(), ptr);
        object_.GetSecond()(ptr);
    }
    void Swap(UniquePtr& other) {
        std::swap(object_.GetFirst(), other.object_.GetFirst());
        std::swap(object_.GetSecond(), other.object_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return object_.GetFirst();
    }
    Deleter& GetDeleter() {
        return object_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return object_.GetSecond();
    }
    explicit operator bool() const {
        return object_.GetFirst();
    }
    T& operator[](size_t i) {
        return *(object_.GetFirst() + i);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    T& operator*() const = delete;
    T* operator->() const = delete;

private:
    CompressedPair<T*, Deleter> object_;
};
