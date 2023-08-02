#pragma once
#include "geo.h"
#include <string>
#include <vector>
#include <string_view>
#include <variant>

namespace transport_catalogue {

	namespace domain {

		struct Stop {
			std::string stop;
			geo::Coordinates coords;
		};

		bool operator<(const Stop& left, const Stop& right);

		struct Bus {
			std::string bus;
			std::vector<const Stop*> route;
			bool is_circle;
		};

		bool operator<(const Bus& left, const Bus& right);

		struct BusInfo {
			std::string_view bus;
			size_t stops;
			size_t unique_stops;
			size_t route_length;
			double curve;
		};

	}

}
