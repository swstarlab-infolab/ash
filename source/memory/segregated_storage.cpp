#include <ash/memory/segregated_storage.h>
#include <assert.h>
#include <ash/pointer.h>

namespace ash {

segregated_storage::segregated_storage(void* preallocated, size_t bufsize_, size_t block_size_) :
    buffer(preallocated), bufsize(bufsize_), block_size(block_size_),
    capacity(bufsize / block_size) {
    reset();
}

void* segregated_storage::allocate() {
    if (!_free_list.empty()) {
        void* p = _free_list.back();
        _free_list.pop_back();
        assert(static_cast<char*>(p) >= static_cast<char*>(buffer) &&
            static_cast<char*>(p) < static_cast<char*>(buffer) + bufsize);
        return p;
    }
    return nullptr;
}

void segregated_storage::deallocate(void* p) {
    assert(static_cast<char*>(p) >= static_cast<char*>(buffer) &&
        static_cast<char*>(p) < static_cast<char*>(buffer) + bufsize);
    _free_list.emplace_back(p);
}

void segregated_storage::reset() {
    _free_list.clear();
    _free_list.resize(capacity);
    void** p = _free_list.data();
    for (size_t i = 0; i < capacity; ++i)
        p[i] = seek_pointer(buffer, block_size * i);
}

double segregated_storage::fill_rate() const {
    return static_cast<double>(_free_list.size()) / static_cast<double>(_free_list.capacity());
}

} // !namespace ash