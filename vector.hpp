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
#include <memory>
#include <iostream>

/*
 * vector
 * small object
 * */
template <typename T, size_t INIT_SO_SIZE_ = 6, typename Alloc = std::allocator<T>,
        typename = typename std::enable_if_t<std::is_default_constructible_v<T>>>
class vector {
public:
    typedef T                                           value_type;
    typedef Alloc                                       allocator_type;
    typedef std::allocator_traits<allocator_type>       allocator_traits_;
    typedef typename allocator_traits_::size_type       size_type;
    typedef typename allocator_traits_::difference_type difference_type;
    typedef T&                                          reference;
    typedef T const&                                    const_reference;
    typedef typename allocator_traits_::pointer         pointer;
    typedef typename allocator_traits_::const_pointer   const_pointer;

private:
    value_type small_[INIT_SO_SIZE_];
    size_type size_ = 0;
    size_type capacity_ = INIT_SO_SIZE_;

    allocator_type alloc_;
    pointer data_ = small_;

    pointer allocate_new_zone_(size_type cap_, size_type sz_, const_pointer src) {
        pointer allocated_ = alloc_.allocate(cap_);
        try {
            std::copy(src, src + sz_, allocated_);
        } catch (...) {
            alloc_.deallocate(allocated_, cap_);
            throw;
        }
        return allocated_;
    }

    void destroy_() {
        std::destroy(data_, data_ + size_);
        if (data_ != small_) {
            alloc_.deallocate(data_, capacity_);
        }
    }

    void set_small_() {
        data_ = small_;
        capacity_ = INIT_SO_SIZE_;
    }

    void set_large_(pointer allocated_, size_type cap_) {
        data_ = allocated_;
        capacity_ = cap_;
    }

    template <typename InputIt,
            typename = typename std::enable_if_t<
                    std::is_constructible_v<typename std::iterator_traits<InputIt>::reference>>>
    vector(InputIt first, InputIt last, std::input_iterator_tag) {
        while (first != last) {
            push_back(*first++);
        }
    }

    template <typename ForwardIt,
            typename = typename std::enable_if_t<
                    std::is_constructible_v<typename std::iterator_traits<ForwardIt>::reference>>>
    vector(ForwardIt first, ForwardIt last, std::forward_iterator_tag) {
        size_type dist = std::distance(first, last);
        if (dist > INIT_SO_SIZE_) {
            pointer allocated_ = alloc_.allocate(dist);
            try {
                std::copy(first, last, allocated_);
            } catch (...) {
                alloc_.deallocate(allocated_, dist);
                throw;
            }
            set_large_(allocated_, dist);
        } else {
            std::copy(first, last, data_);
        }
        size_ = dist;
    }

    template <typename InputIt,
            typename = typename std::enable_if_t<
                    std::is_constructible_v<typename std::iterator_traits<InputIt>::reference>>>
    void assign(InputIt first, InputIt last, std::input_iterator_tag) {
        vector tmp;
        while (first != last) {
            tmp.push_back(*first++);
        }
        swap(tmp);
    }

    template <typename ForwardIt,
            typename = typename std::enable_if_t<
                    std::is_constructible_v<typename std::iterator_traits<ForwardIt>::reference>>>
    void assign(ForwardIt first, ForwardIt last, std::forward_iterator_tag) {
        size_type dist = std::distance(first, last);
        if (dist > INIT_SO_SIZE_) {
            pointer allocated_ = alloc_.allocate(dist);
            try {
                std::copy(first, last, allocated_);
            } catch (...) {
                alloc_.deallocate(allocated_, dist);
                throw;
            }
            destroy_();
            set_large_(allocated_, dist);
        } else {
            if (small()) {
                destroy_();
                std::copy(first, last, small_);
            } else {
                std::copy(first, last, small_);
                destroy_();
            }
            set_small_();
        }
        size_ = dist;
    }

    template <typename Tp>
    void push_back_slow_path_(Tp&& x) {
        pointer allocated_ = allocate_new_zone_(capacity_ << 1, size_, data_);
        try {
            new(allocated_ + size_) T(std::forward<Tp>(x));
        } catch (...) {
            alloc_.deallocate(allocated_, capacity_ << 1);
            throw;
        }
        destroy_();
        set_large_(allocated_, capacity_ << 1);
    }

    template <typename... Args>
    void emplace_back_slow_path_(Args&&... args) {
        pointer allocated_ = allocate_new_zone_(capacity_ << 1, size_, data_);
        try {
            new(allocated_ + size_) T(std::forward<Args>(args)...);
        } catch (...) {
            alloc_.deallocate(allocated_, capacity_ << 1);
            throw;
        }
        destroy_();
        set_large_(allocated_, capacity_ << 1);
    }

public:
    vector() noexcept = default;

    vector(vector const& rhs) {
        if (rhs.size_ <= INIT_SO_SIZE_) {
            std::copy(rhs.data_, rhs.data_ + rhs.size_, data_);
        } else {
            pointer allocated_ = allocate_new_zone_(rhs.size_, rhs.size_, rhs.data_);
            set_large_(allocated_, rhs.size_);
        }
        size_ = rhs.size_;
    }

    vector(vector&& rhs) {
        swap(rhs);
    }

    explicit vector(size_type initial_size, const_reference x = T()) {
        if (initial_size > INIT_SO_SIZE_) {
            pointer allocated_ = alloc_.allocate(initial_size);
            try {
                std::uninitialized_fill(allocated_, allocated_ + initial_size, x);
            } catch (...) {
                alloc_.deallocate(allocated_, initial_size);
                throw;
            }
            set_large_(allocated_, initial_size);
        } else {
            std::uninitialized_fill(data_, data_ + initial_size, x);
        }
        size_ = initial_size;
    }

    template <typename Iterator>
    vector(Iterator first, Iterator last)
            : vector(first, last, typename std::iterator_traits<Iterator>::iterator_category()) {}

    ~vector() noexcept {
        destroy_();
    }

    vector& operator=(vector const& rhs) {
        if (rhs.size_ <= capacity_) {
            destroy_();
            std::copy(rhs.data_, rhs.data_ + rhs.size_, data_);
        } else {
            pointer allocated_ = allocate_new_zone_(rhs.capacity_, rhs.size_, rhs.data_);
            destroy_();
            set_large_(allocated_, rhs.capacity_);
        }
        size_ = rhs.size_;
        return *this;
    }

    vector& operator=(vector&& rhs) {
        swap(rhs);
        return *this;
    }

    vector& swap(vector& rhs) {
        if (small() && rhs.small()) {
            std::swap_ranges(data_, data_ + INIT_SO_SIZE_, rhs.data_);
        } else if (small()) {
            std::move(small_, small_ + size_, rhs.small_);
            std::destroy(small_, small_ + size_);
            set_large_(rhs.data_, rhs.capacity_);
            rhs.set_small_();
        } else if (rhs.small()) {
            rhs.swap(*this);
        } else {
            std::swap(data_, rhs.data_);
            std::swap(capacity_, rhs.capacity_);
        }
        std::swap(size_, rhs.size_);
        return *this;
    }

    void assign(size_type count, const_reference x) {
        if (count <= capacity_) {
            destroy_();
            std::uninitialized_fill(data_, data_ + count, x);
        } else {
            pointer allocated_ = alloc_.allocated(count);
            try {
                std::uninitialized_fill(allocated_, data_ + count, x);
            } catch (...) {
                alloc_.deallocate(allocated_);
                throw;
            }
            destroy_();
            set_large_(allocated_, count);
        }
        size_ = count;
    }

    template <typename Iterator>
    void assign(Iterator first, Iterator last) {
        assign(first, last, typename std::iterator_traits<Iterator>::iterator_category());
    }

    allocator_type get_allocator() const noexcept {
        return alloc_;
    }

    reference at(size_t i) {
        if (i >= size_) {
            throw std::out_of_range("access index out of range");
        }
        return data_[i];
    }

    const_reference at(size_t i) const {
        if (i >= size_) {
            throw std::out_of_range("access index out of range");
        }
        return data_[i];
    }

    reference operator[](size_t i) noexcept {
        return data_[i];
    }

    const_reference operator[](size_t i) const noexcept {
        return data_[i];
    }

    reference front() noexcept {
        return data_[0];
    }

    const_reference front() const noexcept {
        return data_[0];
    }

    reference back() {
        return data_[size_ - 1];
    }

    const_reference back() const noexcept {
        return data_[size_ - 1];
    }

    pointer data() noexcept {
        return data_;
    }

    const_pointer data() const noexcept {
        return data_;
    }

    bool empty() const noexcept {
        return size_ == 0;
    }

    size_type size() const noexcept {
        return size_;
    }

    void reserve(size_type cap_) {
        if (cap_ <= capacity_) {
            return;
        }
        pointer allocated_ = allocate_new_zone_(cap_, size_, data_);
        destroy_();
        set_large_(allocated_, cap_);
    }

    size_type capacity() const noexcept {
        return capacity_;
    }

    void shrink_to_fit() {
        if (capacity_ == size_ || capacity_ == INIT_SO_SIZE_) {
            return;
        }
        if (size_ <= INIT_SO_SIZE_) {
            std::move(data_, data_ + size_, small_);
            destroy_();
            set_small_();
        } else {
            pointer allocated_ = allocate_new_zone_(size_, size_, data_);
            destroy_();
            set_large_(allocated_, size_);
        }
    }

    bool small() const noexcept {
        return data_ == small_;
    }

    void clear() noexcept {
        destroy_();
        set_small_();
        size_ = 0;
    }

    void pop_back() noexcept {
        --size_;
        std::destroy(data_ + size_, data_ + size_ + 1);
    }

    void push_back(const_reference x = T()) {
        if (size_ < capacity_) {
            new (data_ + size_) T(x);
        } else {
            push_back_slow_path_(x);
        }
        ++size_;
    }

    void push_back(value_type&& x) {
        if (size_ < capacity_) {
            new (data_ + size_) T(std::move(x));
        } else {
            push_back_slow_path_(std::move(x));
        }
        ++size_;
    }

    template <typename... Args>
    void emplace_back(std::enable_if_t<std::is_constructible_v<value_type, Args>, Args>&&... args) {
        if (size_ < capacity_) {
            new(data_ + size_) T(std::forward<Args>(args)...);
        } else {
            emplace_back_slow_path_(std::forward<Args>(args)...);
        }
        ++size_;
    }

    void resize(size_type sz_) {
        if (sz_ <= size_) {
            std::destroy(data_ + sz_, data_ + size_);
        } else {
            if (sz_ <= capacity_) {
                std::uninitialized_fill(data_ + size_, data_ + sz_, value_type());
            } else {
                pointer allocated_ = allocate_new_zone_(sz_, size_, data_);
                try {
                    std::uninitialized_fill(allocated_ + size_, allocated_ + sz_, T());
                } catch (...) {
                    alloc_.deallocate(allocated_, sz_);
                    throw;
                }
                destroy_();
                set_large_(allocated_, sz_);
            }
        }
        size_ = sz_;
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



























