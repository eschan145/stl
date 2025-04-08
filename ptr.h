/*
This class was adapted from https://github.com/macmade/CPP-ARC with some fixes
related to invalid delete.
*/

#include <exception>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

namespace stl {

class exception: public std::exception {
    std::string message;
 public:
    exception(const std::string& msg) {
        this->message = msg;
    }

    const char* what() const noexcept override {
        return this->message.c_str();
    }
};

class memory_error : public exception {
public:
    using exception::exception;
};

namespace _memory
{
struct AllocInfo {
    std::size_t ref_count;
    std::size_t allocation_size;
};
    
void* allocate(std::size_t size) {
    std::size_t allocation_size;
    char* memory;
    AllocInfo* infos;
        
    allocation_size = size + sizeof(AllocInfo);
    memory = reinterpret_cast<char*>(malloc(allocation_size));
        
    if (!memory) return nullptr;
        
    infos = reinterpret_cast<AllocInfo*>(memory);
    infos->ref_count = 1;
    infos->allocation_size = size;
        
    return reinterpret_cast<void*>(memory + sizeof(AllocInfo));
}
    
void deallocate(void* p) {
    char* memory;
    AllocInfo* infos;
        
    if (!p) return;
        
    memory = reinterpret_cast<char*>(p);
    memory -= sizeof(AllocInfo);
    infos = reinterpret_cast<AllocInfo*>(memory);
        
    if (infos->ref_count > 0) {
        throw stl::exception(
            "Deallocating an object with a reference count of " +
            std::to_string(infos->ref_count) + "!"
        );
    }
        
    free(memory);
}
    
template<typename T>
void release(T* p) {
    char* mem;
    AllocInfo* infos;
        
    if (!p) return;
        
    mem = reinterpret_cast<char*>(p);
    mem -= sizeof(AllocInfo);
    infos = reinterpret_cast<AllocInfo*>(mem);
        
    infos->ref_count--;
        
    if (infos->ref_count == 0) {
        // Call destructor manually in case the memory hasn't been allocated
        p->~T();
        deallocate(p);
    }
}

template<typename T>
T* retain(T* ptr) {
    char* memory;
    AllocInfo* infos;
        
    if (!ptr) return nullptr;
        
    memory = reinterpret_cast<char*>(ptr);
    memory -= sizeof(AllocInfo);
    infos = reinterpret_cast<AllocInfo*>(memory);
        
    infos->ref_count++;
        
    return ptr;
}

std::size_t getref_count(void* p) {
    char* mem;
    AllocInfo* infos;
        
    if (!p) return 0;
        
    mem = reinterpret_cast<char*>(p);
    mem -= sizeof(AllocInfo);
    infos = reinterpret_cast<AllocInfo*>(mem);
        
    return infos->ref_count;
}

}  // namespace _memory

template <typename T>
class ptr {
 public:
            
    ptr(void) {
        this->data = nullptr;
        this->ref_count = 0;
    }
            
    ptr(T* ptr) {
        this->data = ptr;
        this->ref_count = 0;
                
        if (this->data) {
            this->ref_count++;
                    
            _memory::retain(this->data);
        }
    }
            
    ptr(const ptr<T>& arp) {
        this->data = arp.data;
        this->ref_count = arp.ref_count;
                
        if(this->data) {
            this->ref_count++;
                    
            _memory::retain(this->data);
        }
    }
            
    ~ptr() {
        if (this->data) {
            this->ref_count--;
                    
            _memory::release(this->data);
                    
            if(!this->ref_count) {
                _memory::release(this->data);
            }
        }
    }
            
    ptr<T>& operator=(const ptr<T>& other) {
        if (this == &other) {
            return *(this);
        }
                
        if (this->data == other.data) {
            return *(this);
        }
                
        this->data = other.data;
        this->ref_count = other.ref_count;
                
        if(this->data) {
            this->ref_count++;
                    
            _memory::retain(this->data);
        }
                
        return *(this);
    }
        
    void* operator new(std::size_t size) throw() {
        (void)size;
                
        throw stl::exception("stl::ptr cannot be allocated using new!");
                
        return nullptr;
    }
            
    T& operator*() {
        if (!this->data) {
            throw stl::exception("stl::ptr: Null pointer dereference!");
        }
        return *(this->data);
    }
            
    T* operator->() {
        if (!this->data) {
            throw std::runtime_error("stl::ptr: Null pointer access!");
        }
        return this->data;
    }

 private:
    T* data;
    std::size_t ref_count;
};

}  // namespace stl

void* operator new(std::size_t size) noexcept(false) {
    void * p;
    
    p = stl::_memory::allocate(size);
    
    if (!p) throw std::bad_alloc();
    
    return p;
}

void* operator new(std::size_t size, const std::nothrow_t& nothrow) throw() {
    (void)nothrow;
    
    return stl::_memory::allocate(size);
}

void* operator new[](std::size_t size) noexcept(false) {
    void* p;
    
    p = stl::_memory::allocate(size);
    
    if (!p) throw std::bad_alloc();
    
    return p;
}

void* operator new[](std::size_t size, const std::nothrow_t& nothrow) throw() {
    (void)nothrow;
    
    return stl::_memory::allocate(size);
}

void operator delete(void* ptr) throw() {
    stl::_memory::deallocate(ptr);
}

void operator delete(void* ptr, const std::nothrow_t& nothrow) throw() {
    (void )nothrow;
    
    stl::_memory::deallocate(ptr);
}

void operator delete[](void* ptr) throw() {
    stl::_memory::deallocate(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t& nothrow) throw() {
    (void)nothrow;
    
    stl::_memory::deallocate(ptr);
}
