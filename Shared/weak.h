#pragma once

#include "shared.h"
#include "sw_fwd.h"  // Forward declaration

// https://en.cppreference.com/w/cpp/memory/weak_ptr

template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() {
        block_ = nullptr;
    }
    WeakPtr(std::nullptr_t) : WeakPtr() {
    }

    WeakPtr(const WeakPtr& other) {
        block_ = other.block_;
        if (block_) {
            block_->IncWeak();
        }
    }
    template <typename U>
    WeakPtr(const WeakPtr<U>& other) {
        block_ = other.block_;
        if (block_) {
            block_->IncWeak();
        }
    }
    WeakPtr(WeakPtr&& other) {
        block_ = other.block_;
        other.block_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) {
        block_ = other.block_;
        if (block_ != nullptr) {
            block_->IncWeak();
        }
    }
    template <typename U>
    WeakPtr& operator=(const SharedPtr<U>& other) {
        Reset();
        block_ = other.block_;
        if (block_) {
            block_->IncWeak();
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this == reinterpret_cast<const WeakPtr*>(&other)) {
            return *this;
        }
        Reset();
        block_ = other.block_;
        if (block_) {
            block_->IncWeak();
        }
        return *this;
    }
    WeakPtr& operator=(WeakPtr&& other) {
        Reset();
        block_ = other.block_;
        other.block_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_ == nullptr) {
            return;
        }
        block_->DecWeak();
        if (block_->strong == 0 && block_->weak == 0) {
            block_->DeleteBlock();
        }
        block_ = nullptr;
    }
    void Swap(WeakPtr& other) {
        std::swap(block_, other.block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (block_ == nullptr) {
            return 0;
        }
        return block_->strong;
    }
    bool Expired() const {
        return UseCount() == 0;
    }
    SharedPtr<T> Lock() const {
        SharedPtr<T> res;
        if (block_ != nullptr && block_->strong > 0) {
            block_->IncStr();
            res.block_ = block_;
            res.ptr_ = reinterpret_cast<T*>(block_->GetObj());
        }
        return res;
    }

    ControlBlockBase* block_;
};
