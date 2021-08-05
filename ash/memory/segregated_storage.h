#ifndef ASH_MEMORY_SEGREGATED_STORAGE_H
#define ASH_MEMORY_SEGREGATED_STORAGE_H
#include <ash/config.h>
#include <ash/detail/noncopyable.h>
#include <vector>
#include <mutex>

namespace ash {

class segregated_storage final : noncopyable {
public:
    segregated_storage(void* preallocated, size_t bufsize, size_t block_size);
    void* allocate();
    void deallocate(void* p);
    void reset();
    double fill_rate() const;

    bool empty() const {
        return _free_list.empty();
    }

    bool full() const {
        return size() == capacity;
    }

    size_t size() const {
        return _free_list.size();
    }

    void* const buffer;
    size_t const bufsize;
    size_t const block_size;
    size_t const capacity;

private:
    std::vector<void*> _free_list;
};

template <typename Mutex>
class concurrent_segregated_storage final : noncopyable {
public:
    using lock_type = Mutex;

    concurrent_segregated_storage(void* preallocated, size_t bufsize, size_t block_size) :
        _ss(preallocated, bufsize, block_size) {
    }

    ASH_FORCEINLINE void* allocate() {
        std::lock_guard<lock_type> guard{ _m };
        return _ss.allocate();
    }

    ASH_FORCEINLINE void deallocate(void* p) {
        std::lock_guard<lock_type> guard{ _m };
        return _ss.deallocate(p);
    }

    ASH_FORCEINLINE void reset() {
        std::lock_guard<lock_type> guard{ _m };
        return _ss.reset();
    }

    ASH_FORCEINLINE double fill_rate() const {
        std::lock_guard<lock_type> guard{ _m };
        return _ss.fill_rate();
    }

    ASH_FORCEINLINE bool empty() const {
        std::lock_guard<lock_type> guard{ _m };
        return _ss.empty();
    }

    ASH_FORCEINLINE bool full() const {
        std::lock_guard<lock_type> guard{ _m };
        return _ss.full();
    }

    ASH_FORCEINLINE size_t size() const {
        std::lock_guard<lock_type> guard{ _m };
        return _ss.size();
    }

private:
    lock_type _m;
    segregated_storage _ss;
};

} // !namespace ash

#endif // ASH_MEMORY_SEGREGATED_STORAGE_H
