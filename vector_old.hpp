/*
    author dzhiblavi
 */

#ifndef vector_hpp
#define vector_hpp

#include <cstddef>
#include <vector>
#include <type_traits>
#include <cstring>
#include <algorithm>
#include <iostream>

template<typename T>
T* _allocate_new_zone(T const* __restrict__ _src, size_t size, size_t alloc) {
    auto alloc_data = static_cast<T *> (operator new(alloc * sizeof(T)));
    std::copy(_src, _src + size, alloc_data);
    return alloc_data;
}

/*
 * vector
 * small object
 * */
template<typename T, size_t INIT_SO_SIZE_ = 6>
class vector {
public:
    using value_type = T;
    using reference = T &;
    using const_reference = T const &;
    using pointer = T *;
    using const_pointer = T const *;

private:
    T small_[INIT_SO_SIZE_];
    pointer data_ = small_;

    size_t size_ = 0;
    size_t capacity_ = INIT_SO_SIZE_;

    void _clear() {
        std::destroy(data_, data_ + size_);
        if (data_ != small_) {
            operator delete(data_);
        }
    }

    void _set_unique_large_data_(pointer __restrict__ allocated_, size_t new_capacity_) {
        data_ = allocated_;
        capacity_ = new_capacity_;
    }

    void _set_unique_small_data_() {
        data_ = small_;
        capacity_ = INIT_SO_SIZE_;
    }

    void _push_back_short_path(const_reference x) {
        new(data_ + size_) T(x);
    }

    void _push_back_long_path(const_reference x) {
        size_t new_capacity_ = capacity_ << 1;
        pointer alloc_data_ = _allocate_new_zone(data_, size_, new_capacity_);
        try {
            new(alloc_data_ + size_) T(x);
        } catch (...) {
            std::destroy(alloc_data_, alloc_data_ + size_);
            operator delete(alloc_data_);
            throw;
        }
        _clear();
        _set_unique_large_data_(alloc_data_, new_capacity_);
    }

    void _resize_short_path(size_t new_size_) {
        if (new_size_ > size_) {
            std::uninitialized_fill(data_ + size_, data_ + new_size_, T());
        } else if (new_size_ < size_) {
            std::destroy(data_ + new_size_, data_ + size_);
        }
    }

    void _resize_long_path(size_t new_size_) {
        pointer alloc_data_ = _allocate_new_zone(data_, size_, new_size_);
        try {
            std::uninitialized_fill(alloc_data_ + size_, alloc_data_ + new_size_, T());
        } catch (...) {
            std::destroy(alloc_data_, alloc_data_ + size_);
            operator delete(alloc_data_);
            throw;
        }
        _clear();
        _set_unique_large_data_(alloc_data_, new_size_);
    }

public:
    vector() = default;

    template<typename = typename std::enable_if<std::is_default_constructible_v<T>>::type>
    explicit vector(size_t initial_size_) {
        if (initial_size_ <= INIT_SO_SIZE_) {
            std::uninitialized_fill(data_, data_ + initial_size_, T());
        } else {
            auto alloc_data_ = static_cast<pointer> (operator new(initial_size_ * sizeof(T)));
            try {
                std::uninitialized_fill(alloc_data_, alloc_data_ + initial_size_, T());
            } catch (...) {
                operator delete (alloc_data_);
            }
            _set_unique_large_data_(alloc_data_, initial_size_);
        }
        size_ = initial_size_;
    }

    template<typename ForwardIt>
    vector(ForwardIt first, ForwardIt last) {
        auto size = first - last;
        reserve(size);
        std::for_each(first, last, [this](auto const& x) {
            push_back(x);
        });
    }

    ~vector() noexcept {
        _clear();
    }

    vector(vector &&rhs) {
        swap(rhs);
    }

    vector(vector const &rhs) {
        if (rhs.size_ <= INIT_SO_SIZE_) {
            std::copy(rhs.data_, rhs.data_ + rhs.size_, data_);
        } else {
            pointer alloc_data = _allocate_new_zone(rhs.data_, rhs.size_, rhs.size_);
            _set_unique_large_data_(alloc_data, rhs.size_);
        }
        size_ = rhs.size_;
    }

    vector& swap(vector &v) {
        if (small() && v.small()) {
            std::swap_ranges(data_, data_ + INIT_SO_SIZE_, v.data_);
        } else if (small()) {
            std::copy(small_, small_ + size_, v.small_);
            std::swap(data_, v.data_);
            capacity_ = v.capacity_;
            v._set_unique_small_data_();
        } else if (v.small()) {
            v.swap(*this);
        } else {
            std::swap(data_, v.data_);
            std::swap(capacity_, v.capacity_);
        }
        std::swap(size_, v.size_);
        return *this;
    }

    vector& operator=(vector&& rhs) {
        swap(rhs);
        return *this;
    }

    vector& operator=(vector const &rhs) {
        if (rhs.size_ <= INIT_SO_SIZE_) {
            std::copy(rhs.data_, rhs.data_ + rhs.size_, small_);
            _clear();
            _set_unique_small_data_();
        } else {
            pointer alloc_data = _allocate_new_zone(rhs.data_, rhs.size_, rhs.size_);
            _clear();
            _set_unique_large_data_(alloc_data, rhs.size_);
        }
        size_ = rhs.size_;
        return *this;
    }

    void push_back(const_reference x) {
        if (size_ < capacity_) {
            _push_back_short_path(x);
        } else {
            _push_back_long_path(x);
        }
        ++size_;
    }

    void pop_back() noexcept {
        --size_;
        data_[size_].~T();
    }

    void resize(size_t newsize_) {
        if (newsize_ <= capacity_) {
            _resize_short_path(newsize_);
        } else {
            _resize_long_path(newsize_);
        }
        size_ = newsize_;
    }

    void reserve(size_t new_cap) {
        if (new_cap < capacity_) {
            return;
        }
        pointer alloc_data = _allocate_new_zone(data_, size_, new_cap);
        _clear();
        _set_unique_large_data_(alloc_data, new_cap);
    }

    void clear() {
        std::destroy(data_, data_ + size_);
        size_ = 0;
    }

    reference front() noexcept {
        return data_[0];
    }

    const_reference front() const noexcept {
        return data_[0];
    }

    reference back() noexcept {
        return data_[size_ - 1];
    }

    const_reference back() const noexcept {
        return data_[size_ - 1];
    }

    reference operator[](size_t i) {
        return data_[i];
    }

    const_reference operator[](size_t i) const noexcept {
        return data_[i];
    }

    pointer data() noexcept {
        return data_;
    }

    const_pointer data() const noexcept {
        return data_;
    }

    size_t size() const noexcept {
        return size_;
    }

    size_t capacity() const noexcept {
        return capacity_;
    }

    bool empty() const noexcept {
        return !size_;
    }

    bool small() const noexcept {
        return data_ == small_;
    }

    struct const_iterator : std::iterator<std::random_access_iterator_tag, value_type,
            std::ptrdiff_t, const_pointer, const_reference> {
        const_iterator() = default;

        const_iterator(const_iterator const& rhs) = default;
        const_iterator& operator=(const_iterator const& rhs) = default;

        bool operator==(const_iterator const& rhs) {
            return ptr_ == rhs.ptr_;
        }

        bool operator!=(const_iterator const& rhs) {
            return ptr_ != rhs.ptr_;
        }

        const_iterator& operator++() {
            ++ptr_;
            return *this;
        }

        const_iterator& operator--() {
            --ptr_;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator ret(*this);
            ++*this;
            return ret;
        }

        const_iterator operator--(int) {
            const_iterator ret(*this);
            --*this;
            return ret;
        }

        const_reference operator*() {
            return *ptr_;
        }

        const_pointer operator->() {
            return ptr_;
        }

        bool operator<(const_iterator const& rhs) {
            return ptr_ < rhs.ptr_;
        }

        bool operator>(const_iterator const& rhs) {
            return ptr_ > rhs.ptr_;
        }

        bool operator<=(const_iterator const& rhs) {
            return ptr_ <= rhs.ptr_;
        }

        bool operator>=(const_iterator const& rhs) {
            return ptr_ >= rhs.ptr_;
        }

        const_iterator& operator+=(size_t n) {
            ptr_ += n;
            return *this;
        }

        const_iterator& operator-=(size_t n) {
            ptr_ -= n;
            return *this;
        }

        const_reference operator[](size_t n) {
            return ptr_[n];
        }

        friend std::ptrdiff_t operator-(const_iterator const& p, const_iterator const& q) {
            return p.ptr_ - q.ptr_;
        }

        friend const_iterator operator+(const_iterator p, size_t n) {
            return p += n;
        }

        friend const_iterator operator-(const_iterator p, size_t n) {
            return p -= n;
        }

        friend const_iterator operator+(size_t n, const_iterator const& p) {
            return p + n;
        }

        friend const_iterator vector::begin() const;
        friend const_iterator vector::end() const;

    private:
        const_iterator(const_pointer ptr) : ptr_(ptr) {}

        const_pointer ptr_;
    };

    struct iterator : std::iterator<std::random_access_iterator_tag, value_type,
            std::ptrdiff_t, pointer, reference> {
        iterator() = default;

        iterator(iterator const& rhs) = default;
        iterator& operator=(iterator const& rhs) = default;

        bool operator==(iterator const& rhs) {
            return ptr_ == rhs.ptr_;
        }

        bool operator!=(iterator const& rhs) {
            return ptr_ != rhs.ptr_;
        }

        iterator& operator++() {
            ++ptr_;
            return *this;
        }

        iterator& operator--() {
            --ptr_;
            return *this;
        }

        iterator operator++(int) {
            iterator ret(*this);
            ++*this;
            return ret;
        }

        iterator operator--(int) {
            iterator ret(*this);
            --*this;
            return ret;
        }

        reference operator*() {
            return *ptr_;
        }

        pointer operator->() {
            return ptr_;
        }

        bool operator<(iterator const& rhs) {
            return ptr_ < rhs.ptr_;
        }

        bool operator>(iterator const& rhs) {
            return ptr_ > rhs.ptr_;
        }

        bool operator<=(iterator const& rhs) {
            return ptr_ <= rhs.ptr_;
        }

        bool operator>=(iterator const& rhs) {
            return ptr_ >= rhs.ptr_;
        }

        iterator& operator+=(size_t n) {
            ptr_ += n;
            return *this;
        }

        iterator& operator-=(size_t n) {
            ptr_ -= n;
            return *this;
        }

        reference operator[](size_t n) {
            return ptr_[n];
        }

        friend std::ptrdiff_t operator-(iterator const& p, iterator const& q) {
            return p.ptr_ - q.ptr_;
        }

        friend iterator operator+(iterator p, size_t n) {
            return p += n;
        }

        friend iterator operator-(iterator p, size_t n) {
            return p -= n;
        }

        friend iterator operator+(size_t n, iterator const& p) {
            return p + n;
        }

        friend iterator vector::begin();
        friend iterator vector::end();

    private:
        iterator(pointer ptr) : ptr_(ptr) {}

        pointer ptr_ = nullptr;
    };

    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = std::reverse_iterator<iterator>;

    const_iterator begin() const {
        return const_iterator(data_);
    }

    const_iterator end() const {
        return const_iterator(data_ + size_);
    }

    const_reverse_iterator rbegin() const {
        if (empty()) {
            return const_reverse_iterator();
        }
        return const_reverse_iterator(end());
    }

    const_reverse_iterator rend() const {
        if (empty()) {
            return const_reverse_iterator();
        }
        return const_reverse_iterator(begin());
    }

    iterator begin() {
        return iterator(data_);
    }

    iterator end() {
        return iterator(data_ + size_);
    }

    reverse_iterator rbegin() {
        if (empty()) {
            return reverse_iterator();
        }
        return reverse_iterator(end());
    }

    reverse_iterator rend() {
        if (empty()) {
            return reverse_iterator();
        }
        return reverse_iterator(begin());
    }
};


#endif /* vector.hpp */
