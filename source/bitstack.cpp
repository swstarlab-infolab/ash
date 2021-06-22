#include <ash/bitstack.h>
#include <ash/bitset.h>
#include <ash/size.h>
#include <assert.h>
#include <stdlib.h>

namespace ash {

bitstack::bitstack(size_t capacity_) {
#ifdef _MSC_VER
    _cap = roundup(capacity_, 4llu);
#else
    _cap = roundup(capacity_, 4lu);
#endif
    _top = 0;
    _container = static_cast<int*>(malloc(_cap / sizeof(int)));
    if (_container != nullptr)
        memset(_container, 0, _cap / sizeof(int));
}

bitstack::bitstack(bitstack const& other) {
    _cap = other._cap;
    _top = other._top;
    _container = static_cast<int*>(malloc(_cap / sizeof(int)));
    if (_container != nullptr)
        memcpy(_container, other._container, _cap / sizeof(int));
}

bitstack::bitstack(bitstack&& other) noexcept {
    _cap = other._cap;
    _top = other._top;
    _container = other._container;
    other._container = nullptr;
}


bitstack::~bitstack() noexcept {
    free(_container);
}

bitstack& bitstack::operator=(bitstack const& rhs) {
    _cap = rhs._cap;
    _top = rhs._top;
    _container = static_cast<int*>(malloc(_cap / sizeof(int)));
    if (_container != nullptr)
        memcpy(_container, rhs._container, _cap / sizeof(int));
    return *this;
}

bitstack& bitstack::operator=(bitstack&& rhs) noexcept {
    _cap = rhs._cap;
    _top = rhs._top;
    _container = rhs._container;
    rhs._container = nullptr;
    return *this;
}

bool bitstack::push(bool const value) {
    if (_top == _cap) {
        if (!reserve(_cap * 2))
            return false;
        _top += 1;
    }
    if (value)
        set_bit(_container, _top);
    else
        clear_bit(_container, _top);
    _top += 1;
    return true;
}

bool bitstack::pop() {
    if (empty())
        return false;
    _top -= 1;
    return true;
}

bool bitstack::peek() const {
    assert(_top > 0);
    return test_bit(_container, _top - 1);
}

void bitstack::clear() {
    _top = 0;
    if (_container != nullptr)
        memset(_container, 0, _cap / sizeof(int));
}

bool bitstack::reserve(size_t const new_capacity) {
    if (_cap >= new_capacity)
        return true;
    assert(_cap % 4 == 0);
    assert(_container != nullptr);
    int* new_container = static_cast<int*>(malloc(new_capacity / sizeof(int)));
    if (new_container == nullptr)
        return false;
    memcpy(new_container, _container, size() / sizeof(int));
    free(_container);
    _container = new_container;
    return true;
}

}
