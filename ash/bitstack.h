#ifndef ASH_BITSTACK_H
#define ASH_BITSTACK_H
#include <stddef.h>

namespace ash {

class bitstack {
public:
    explicit bitstack(size_t capacity_ = 128);
    bitstack(bitstack const& other);
    bitstack(bitstack&& other) noexcept;
    ~bitstack() noexcept;
    bitstack& operator=(bitstack const& rhs);
    bitstack& operator=(bitstack&& rhs) noexcept;
    bool push(bool value);
    bool pop();
    bool peek() const;
    void clear();
    bool reserve(size_t new_capacity);

    bool push(unsigned const value) {
        return push(value != 0);
    }

    bool push(int const value) {
        return push(value != 0);
    }

    size_t size() const {
        return _top;
    }

    bool empty() const {
        return _top == 0;
    }

protected:
    size_t _cap;
    size_t _top;
    int* _container;
};

} // !namespace ash
#endif // ASH_BITSTACK_H
