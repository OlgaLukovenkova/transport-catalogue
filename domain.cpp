#include "domain.h"

namespace transport_catalogue {

	namespace domain {
		bool operator<(const Stop& left, const Stop& right) {
			return left.stop < right.stop;
		}

		bool operator<(const Bus& left, const Bus& right) {
			return left.bus < right.bus;
		}
	}
}