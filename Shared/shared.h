#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <utility>

struct ControlBlockBase {
    size_t strong = 1;
    size_t weak = 1;

    ControlBlockBase() = default;

    void IncStr() {
        ++strong;
    }

    void DecStr() {
        --strong;
    }

    void IncWeak() {
        ++weak;
    }

    void DecWeak() {
        --weak;
    }

    virtual void* GetObj() noexcept = 0;

    virtual void DestroyObject() noexcept = 0;

    virtual void DeleteBlock() noexcept = 0;

    virtual ~ControlBlockBase() = default;
};

template <typename T>
struct ControlBlockPtr : ControlBlockBase {
    T* ptr;

    ControlBlockPtr(T* p) : ptr(p) {
    }

    void DestroyObject() noexcept override {
        if (ptr) {
            delete ptr;
        }
        ptr = nullptr;
    }

    void DeleteBlock() noexcept override {
        delete this;
    }

    void* GetObj() noexcept override {
        return reinterpret_cast<void*>(ptr);
    }
};

template <typename T>
struct ControlBlockInplace : ControlBlockBase {
    alignas(T) unsigned char storage[sizeof(T)];

    template <typename... Args>
    explicit ControlBlockInplace(Args&&... args) {
        ::new (static_cast<void*>(storage)) T(std::forward<Args>(args)...);
    }

    void* GetObj() noexcept override {
        return reinterpret_cast<void*>(storage);
    }

    void DestroyObject() noexcept override {
        T* obj = reinterpret_cast<T*>(GetObj());
        obj->~T();
    }

    void DeleteBlock() noexcept override {
        delete this;
    }
};

class EnableSharedFromThisBase {
public:
    // virtual void PropagateShared(ControlBlockBase* cb) noexcept = 0;
    // virtual ~EnableSharedFromThisBase() = default;
};

template <typename T>
class EnableSharedFromThis : public EnableSharedFromThisBase {
public:
    WeakPtr<T> weak_this;

    // void PropagateShared(ControlBlockBase* cb) noexcept override {
    //     weak_this.block_ = cb;
    //     if (cb) {
    //         cb->IncWeak();
    //     }
    // }

    SharedPtr<T> SharedFromThis() {
        if (weak_this.UseCount() > 0) {
            return SharedPtr<T>(weak_this);
        } else {
            throw BadWeakPtr{};
        }
    }
    SharedPtr<const T> SharedFromThis() const {
        if (weak_this.UseCount() > 0) {
            return SharedPtr<const T>(weak_this);
        } else {
            throw BadWeakPtr{};
        }
    }

    WeakPtr<T> WeakFromThis() noexcept {
        if (weak_this.UseCount() > 0) {
            return WeakPtr<T>(weak_this);
        } else {
            return nullptr;
        }
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        if (weak_this.UseCount() > 0) {
            return WeakPtr<const T>(weak_this);
        } else {
            return nullptr;
        }
    }
};
// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    template <typename Y>
    void InitWeakThis(EnableSharedFromThis<Y>* e) {
        e->weak_this = *this;
    }

    SharedPtr() {
        ptr_ = nullptr;
        block_ = nullptr;
    }
    SharedPtr(std::nullptr_t) : SharedPtr() {
    }

    template <typename U>
    explicit SharedPtr(U* ptr) {
        block_ = new ControlBlockPtr<U>(ptr);
        ptr_ = static_cast<T*>(ptr);
        // assert(std::is_convertible_v<T*, EnableSharedFromThisBase*>);
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitWeakThis(ptr_);
        }
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    SharedPtr(const SharedPtr<U>& other) {
        block_ = other.block_;
        ptr_ = static_cast<T*>(other.ptr_);
        if (block_ != nullptr) {
            block_->IncStr();
        }
    }
    SharedPtr(const SharedPtr& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_ != nullptr) {
            block_->IncStr();
        }
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    SharedPtr(SharedPtr<U>&& other) {
        block_ = other.block_;
        ptr_ = static_cast<T*>(other.ptr_);
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }
    SharedPtr(SharedPtr&& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        block_ = other.block_;
        ptr_ = ptr;
        if (block_) {
            block_->IncStr();
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        block_ = other.block_;
        ptr_ = nullptr;
        if (block_) {
            if (block_->strong == 0) {
                throw BadWeakPtr{};
            }
            block_->IncStr();
            ptr_ = reinterpret_cast<T*>(block_->GetObj());
        } else {
            throw BadWeakPtr{};
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this == reinterpret_cast<const SharedPtr*>(&other)) {
            return *this;
        }
        Reset();
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_ != nullptr) {
            block_->IncStr();
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) {
        Reset();
        ptr_ = other.ptr_;
        block_ = other.block_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_ != nullptr) {
            block_->DecStr();
            if (block_->strong == 0) {
                block_->DestroyObject();
                block_->DecWeak();
            }
            if (block_ != nullptr && block_->strong == 0 && block_->weak == 0) {
                block_->DeleteBlock();
            }
            ptr_ = nullptr;
            block_ = nullptr;
        }
    }
    template <typename U>
    void Reset(U* ptr) {
        Reset();
        block_ = new ControlBlockPtr<U>(ptr);
        ptr_ = static_cast<T*>(ptr);
    }
    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    }
    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }
    size_t UseCount() const {
        if (block_ == nullptr) {
            return 0;
        }
        return block_->strong;
    }
    explicit operator bool() const {
        return ptr_;
    }

    T* ptr_;
    ControlBlockBase* block_;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.block_ == right.block_;
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    SharedPtr<T> res;
    res.block_ = new ControlBlockInplace<T>(std::forward<Args>(args)...);
    res.ptr_ = reinterpret_cast<T*>(res.block_->GetObj());

    if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
        res.InitWeakThis(res.ptr_);
    }
    return res;
}
