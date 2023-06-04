#include <ash/benchmark/time_series_logger.h>

namespace ash {

void time_series_log::write_to_stream(std::ostream& os) const {
       for (auto const& period : _periods) {
           os << period.start.time_since_epoch().count() << ','
           << period.end.time_since_epoch().count() << '\n';
    }
}

} // !namespace ash
