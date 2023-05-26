#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <stdexcept>
#include <memory>
#include <algorithm>
#include <numeric>
#include <iostream>


template<typename T>
class RawMemory {
public:
    RawMemory() = default;
    explicit RawMemory(size_t capacity);
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept;
    RawMemory& operator=(RawMemory&& rhs) noexcept;
    ~RawMemory();

public:
    T* operator+(size_t offset) noexcept;
    const T* operator+(size_t offset) const noexcept;
    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;
    void Swap(RawMemory& other) noexcept;
    const T* GetAddress() const noexcept;
    T* GetAddress() noexcept;
    size_t Capacity() const;

private:
    T* buffer_ = nullptr;
    size_t capacity_ = 0;

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n);
    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept;
};


template<typename T>
class Vector {
public:
    Vector() = default;
    explicit Vector(size_t size);
    Vector(const Vector& other);
    Vector& operator=(const Vector& rhs);
    Vector(Vector&& other) noexcept;
    Vector& operator=(Vector&& rhs) noexcept;
    ~Vector();


public:
    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;
    
    template<typename... Args>
    iterator Emplace(const_iterator pos, Args&& ... args);
    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/;
    template<typename S>
    iterator Insert(const_iterator pos, S&& value);

public:
    void Resize(size_t new_size);
    template<typename... Args>
    T& EmplaceBack(Args&& ... args);
    template<typename S>
    void PushBack(S&& value);
    void PopBack() noexcept;
    void Swap(Vector& other) noexcept;
    void Reserve(size_t new_capacity);
    size_t Size() const noexcept;
    size_t Capacity() const noexcept;
    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;


private:
    RawMemory<T> data_;
    size_t size_ = 0;
};


template<typename T>
RawMemory<T>::RawMemory(size_t capacity) : buffer_(Allocate(capacity)), capacity_(capacity) {}


template<typename T>
RawMemory<T>::RawMemory(RawMemory&& other) noexcept: buffer_(other.buffer_), capacity_(other.capacity_) {
    other.buffer_ = nullptr;
    other.capacity_ = 0;
}


template<typename T>
RawMemory<T>& RawMemory<T>::operator=(RawMemory&& rhs) noexcept {
    Swap(rhs);
    return *this;
}


template<typename T>
RawMemory<T>::~RawMemory() {
    Deallocate(buffer_);
}


template<typename T>
T* RawMemory<T>::operator+(size_t offset) noexcept {
    // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
    assert(offset <= capacity_);
    return buffer_ + offset;
}


template<typename T>
const T* RawMemory<T>::operator+(size_t offset) const noexcept {
    return const_cast<RawMemory&>(*this) + offset;
}


template<typename T>
const T& RawMemory<T>::operator[](size_t index) const noexcept {
    return const_cast<RawMemory&>(*this)[index];
}


template<typename T>
T& RawMemory<T>::operator[](size_t index) noexcept {
    assert(index < capacity_);
    return buffer_[index];
}


template<typename T>
void RawMemory<T>::Swap(RawMemory& other) noexcept {
    std::swap(buffer_, other.buffer_);
    std::swap(capacity_, other.capacity_);
}


template<typename T>
const T* RawMemory<T>::GetAddress() const noexcept {
    return buffer_;
}


template<typename T>
T* RawMemory<T>::GetAddress() noexcept {
    return buffer_;
}


template<typename T>
size_t RawMemory<T>::Capacity() const {
    return capacity_;
}


template<typename T>
T* RawMemory<T>::Allocate(size_t n) {
    return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
}


template<typename T>
void RawMemory<T>::Deallocate(T* buf) noexcept {
    if (buf) {
        operator delete(buf);
    }
}


template<typename T>
Vector<T>::Vector(size_t size) : data_(size), size_(size) {
    std::uninitialized_value_construct_n(data_.GetAddress(), size);
}


template<typename T>
Vector<T>::Vector(const Vector& other) : data_(other.size_), size_(other.size_) {
    std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
}


template<typename T>
Vector<T>& Vector<T>::operator=(const Vector& rhs) {
    if (rhs.size_ > data_.Capacity()) {
        Vector<T> new_vec(rhs);
        Swap(new_vec);
    }
    else { // rhs.size <= data_.Capacity();
        if (rhs.size_ > size_) {
            std::copy_n(rhs.data_.GetAddress(), size_, data_.GetAddress());
            std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
        }
        else {
            std::copy_n(rhs.data_.GetAddress(), rhs.size_, data_.GetAddress());
            std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
        }
        size_ = rhs.size_;
    }
    return *this;
}


template<typename T>
Vector<T>::Vector(Vector&& other) noexcept: data_(std::move(other.data_)), size_(other.size_) {
    other.size_ = 0;
}


template<typename T>
Vector<T>& Vector<T>::operator=(Vector&& rhs) noexcept {
    if (this != &rhs) {
        Swap(rhs);
    }
    return *this;
}


template<typename T>
Vector<T>::~Vector() {
    std::destroy_n(data_.GetAddress(), size_);
}


template<typename T>
template<typename S>
typename Vector<T>::iterator Vector<T>::Insert(Vector::const_iterator pos, S&& value) {
    return Emplace(pos, std::forward<S>(value));
}


template<typename T>
typename Vector<T>::iterator Vector<T>::Erase(Vector::const_iterator pos) {
    assert(pos >= begin() && pos < end());
    size_t pos_distance = std::distance(cbegin(), pos);
    
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::move(data_.GetAddress() + pos_distance + 1, data_.GetAddress() + size_, data_.GetAddress() + pos_distance);
    }
    else {
        std::copy(data_.GetAddress() + pos_distance + 1, data_.GetAddress() + size_, data_.GetAddress() + pos_distance);
    }
    std::destroy_at(data_.GetAddress() + size_ - 1);
    --size_;
    return begin() + pos_distance;
}


template<typename T>
template<typename... Args>
typename Vector<T>::iterator Vector<T>::Emplace(Vector::const_iterator pos, Args&& ... args) {
    assert(pos >= begin() && pos <= end());
    size_t pos_distance = std::distance(cbegin(), pos);
    
    if (data_.Capacity() == size_) {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        
        new(new_data.GetAddress() + pos_distance) T(std::forward<Args>(args)...);
        
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), pos_distance, new_data.GetAddress());
            std::uninitialized_move_n(data_.GetAddress() + pos_distance, size_ - pos_distance,
                    new_data.GetAddress() + pos_distance + 1);
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), pos_distance, new_data.GetAddress());
            std::uninitialized_copy_n(data_.GetAddress() + pos_distance, size_ - pos_distance,
                    new_data.GetAddress() + pos_distance + 1);
        }
        std::destroy_n(data_.GetAddress(), size_);
        
        data_.Swap(new_data);
        ++size_;
    }
    else {
        //new (data_.GetAddress() + size_) T();
        if (pos != data_.GetAddress() + size_) {
            
            T new_type(std::forward<Args>(args)...);
            
            new (data_.GetAddress() + size_) T(std::forward<T>(data_[size_ - 1]));
            
            std::move_backward(data_.GetAddress() + pos_distance, data_.GetAddress() + size_ - 1,
                    data_.GetAddress() + size_);
            *(data_.GetAddress() + pos_distance) = std::move(new_type);
        }
        else {
            new(data_.GetAddress() + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
    }
    return begin() + pos_distance;
}


//region Iterators


template<typename T>
typename Vector<T>::iterator Vector<T>::begin() noexcept {
    return data_.GetAddress();
}


template<typename T>
typename Vector<T>::iterator Vector<T>::end() noexcept {
    return data_.GetAddress() + size_;
}


template<typename T>
typename Vector<T>::const_iterator Vector<T>::cbegin() const noexcept {
    return data_.GetAddress();
}


template<typename T>
typename Vector<T>::const_iterator Vector<T>::cend() const noexcept {
    return data_.GetAddress() + size_;
}


template<typename T>
typename Vector<T>::const_iterator Vector<T>::begin() const noexcept {
    return cbegin();
}


template<typename T>
typename Vector<T>::const_iterator Vector<T>::end() const noexcept {
    return cend();
}


//endregion


template<typename T>
void Vector<T>::Resize(size_t new_size) {
    if (new_size > size_) {
        Reserve(new_size);
        std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
    }
    else {
        std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
    }
    size_ = new_size;
}


template<typename T>
template<typename... Args>
T& Vector<T>::EmplaceBack(Args&& ... args) {
    if (data_.Capacity() == size_) {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        new(new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
        
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    else {
        new(data_.GetAddress() + size_) T(std::forward<Args>(args)...);
    }
    ++size_;
    return data_[size_ - 1];
}


template<typename T>
template<typename S>
void Vector<T>::PushBack(S&& value) {
    if (data_.Capacity() == size_) {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        new(new_data.GetAddress() + size_) T(std::forward<S>(value));
        
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    else {
        new(data_.GetAddress() + size_) T(std::forward<S>(value));
    }
    ++size_;
}


template<typename T>
void Vector<T>::PopBack() noexcept {
    std::destroy_at(data_.GetAddress() + size_ - 1);
    --size_;
}


template<typename T>
void Vector<T>::Swap(Vector& other) noexcept {
    data_.Swap(other.data_);
    std::swap(size_, other.size_);
}


template<typename T>
void Vector<T>::Reserve(size_t new_capacity) {
    if (new_capacity <= data_.Capacity()) {
        return;
    }
    
    RawMemory<T> new_data(new_capacity);
    
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
    }
    else {
        std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
    }
    
    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
}


template<typename T>
size_t Vector<T>::Size() const noexcept {
    return size_;
}


template<typename T>
size_t Vector<T>::Capacity() const noexcept {
    return data_.Capacity();
}


template<typename T>
const T& Vector<T>::operator[](size_t index) const noexcept {
    return const_cast<Vector&>(*this)[index];
}


template<typename T>
T& Vector<T>::operator[](size_t index) noexcept {
    assert(index < size_);
    return data_[index];
}