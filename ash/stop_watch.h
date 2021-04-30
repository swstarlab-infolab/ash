#ifndef ASH_STOP_WATCH_H
#define ASH_STOP_WATCH_H 
#include <chrono>

namespace ash {

class stop_watch {
public:
    using clock = std::chrono::high_resolution_clock;
    using microseconds = std::chrono::microseconds;
    using milliseconds = std::chrono::milliseconds;
    using seconds = std::chrono::seconds;
    using time_point = clock::time_point;

    stop_watch() : _start(clock::now()) {
        static_assert(std::chrono::steady_clock::is_steady, "Serious OS/C++ library issues. Steady clock is not steady");
        // FYI:  This would fail  static_assert(std::chrono::high_resolution_clock::is_steady(), "High Resolution Clock is NOT steady on CentOS?!");
    }

    uint64_t elapsed_us() const {
        return std::chrono::duration_cast<microseconds>(clock::now() - _start).count();
    }

    uint64_t elapsed_ms() const {
        return std::chrono::duration_cast<milliseconds>(clock::now() - _start).count();
    }

    double elapsed_sec() const {
        return std::chrono::duration<double>(clock::now() - _start).count();
    }

    time_point restart() {
        _start = clock::now();
        return _start;
    }

private:
    time_point _start;
};


} // !namespace ash

#endif // !ASH_STOP_WATCH_H