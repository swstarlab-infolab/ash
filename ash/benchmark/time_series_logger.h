#ifndef ASH_BENCHMARK_TIME_SERIES_LOG_H
#define ASH_BENCHMARK_TIME_SERIES_LOG_H
#include <algorithm>
#include <chrono>
#include <ostream>
#include <vector>

namespace ash {

struct time_period {
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point end;
};

class time_series_log {
public:
    using clock_type = std::chrono::high_resolution_clock;
    using time_point = clock_type::time_point;

    time_series_log() = default;
    ~time_series_log() = default;

    time_series_log(time_series_log const& other) {
               _periods = other._periods;
    }

    void add(time_period const& period) {
        _periods.push_back(period);
    }

    void add(clock_type::time_point const& start, clock_type::time_point const& end) {
        _periods.push_back({ start, end });
    }

    void write_to_stream(std::ostream& os) const;

    void operator+=(time_series_log const& other) {
        _periods.insert(_periods.end(), other._periods.begin(), other._periods.end());
        _periods = _merge_periods(_periods);
    }

    time_series_log& operator=(time_series_log const& other) {
        _periods = other._periods;
        return *this;
    }

private:
    static bool _compare(const time_period& a, const time_period& b) {
        return a.start < b.start;
    }

    static std::vector<time_period> _merge_periods(std::vector<time_period>& periods) {
        if (periods.empty()) return periods;

        // Step 1: Sort the periods
        std::sort(periods.begin(), periods.end(), _compare);

        std::vector<time_period> result;
        result.push_back(periods[0]);

        for (size_t i = 1; i < periods.size(); i++) {
            // Step 2: Check for overlap
            if (periods[i].start <= result.back().end) {
                // Step 3: Merge the periods
                if (periods[i].end > result.back().end) {
                    result.back().end = periods[i].end;
                }
            }
            else {
                result.push_back(periods[i]);
            }
        }

        return result;
    }

    std::vector<time_period> _periods;
};

} // !namespace ash

#endif // !ASH_BENCHMARK_TIME_SERIES_LOG_H
