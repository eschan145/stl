#include <vector>

#include "exception.h"

namespace stl {

template<typename T>
class vector {
 private:
    std::vector<T> data;

 public:
    vector() = default;
    vector(size_t size, const T& value = T()) {
        this->data(size, value);
    }
    T& operator[](size_t index) {

#ifndef NDEBUG
        if (index >= this->size()) {
            throw stl::out_of_range("List index out of range!");
        }
#endif
        return this->data[index];
    }

    void push_back(const T& value) {
        this->data.push_back(value);
    }

    void push_back(T&& value) {
        this->data.push_back(std::move(value));
    }

    void reserve(size_t new_capacity) {
        if (new_capacity <= this->data.capacity()) {
            return;
        }
        this->data.reserve(new_capacity);
    }

    size_t capacity() const {
        return this->data.capacity();
    }

    size_t size() const {
        return this->data.size();
    }

    typename std::vector<T>::iterator begin() {
        return this->data.begin();
    }

    typename std::vector<T>::iterator end() {
        return this->data.end();
    }

    void clear() {
        this->data.clear();
    }
};

}  // namespace stl
