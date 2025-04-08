#include <stdexcept>
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

class out_of_range : public exception {
 public:
    using exception::exception;
}

}  // namespace stl
