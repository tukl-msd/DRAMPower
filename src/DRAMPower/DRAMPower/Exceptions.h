#ifndef DRAMPOWER_EXCEPTIONS_H
#define DRAMPOWER_EXCEPTIONS_H

#include <exception>
#include <string>


namespace DRAMPower {

class Exception : public std::exception {
// protected constructors
protected:

// public constructors
public:
    // Constructors with message
    explicit Exception(const std::string &message) : m_message(message) {}
    explicit Exception(std::string &&message) : m_message(std::move(message)) {}
    
    // Exception without message should not be allowed
    Exception() = delete;
    
    // Copy and move constructors and assignment operators
    Exception(const Exception &other) = default;
    Exception &operator=(const Exception &other) = default;
    Exception(Exception &&other) = default;
    Exception &operator=(Exception &&other) = default;

    // Destructor
    virtual ~Exception() noexcept = default;

// Public member functions
public:
    const char *what() const noexcept override {
        return m_message.c_str();
    }

// Private member variables
private:
    std::string m_message;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_EXCEPTIONS_H */
