#include "transport_catalogue.h"

namespace transport_catalogue {

	using namespace std;
	using namespace domain;
	using namespace geo;

	void TransportCatalogue::AddStop(const Stop& stop) {
		stops_.push_back(stop);
		const Stop& added_stop = stops_.back();
		stops_by_name_[added_stop.stop] = &added_stop;
		buses_by_stop_[&added_stop] = {};
	}

	void TransportCatalogue::AddBus(const Bus& bus) {
		buses_.push_back(bus);
		const Bus& added_bus = buses_.back();
		buses_by_name_[added_bus.bus] = &added_bus;
		for (auto stop : bus.route) {
			buses_by_stop_[stop].insert(added_bus.bus);
		}
	}

	optional<const Bus*> TransportCatalogue::FindBusByName(string_view bus) const {
		if (buses_by_name_.count(bus) == 0) {
			return nullopt;
		}
		return { buses_by_name_.at(bus) };
	}

	optional<const Stop*> TransportCatalogue::FindStopByName(string_view stop) const {
		if (stops_by_name_.count(stop) == 0) {
			return nullopt;
		}
		return { stops_by_name_.at(stop) };
	}

	optional<BusInfo> TransportCatalogue::GetInfoAboutBus(string_view bus) const {
		auto opt = FindBusByName(bus);
		if (!opt) {
			return nullopt;
		}

		const Bus& found_bus = *opt.value();
		size_t stops = 0u;
		size_t unique_stops = 0u;

		double geo_length = 0.0;
		size_t route_length = 0u;

		for (size_t i = 0u; i < found_bus.route.size() - 1; ++i) {
			geo_length += ComputeDistance(found_bus.route.at(i)->coords, found_bus.route.at(i + 1)->coords);
			route_length += GetDistance(found_bus.route.at(i)->stop, found_bus.route.at(i + 1)->stop);
		}

		if (found_bus.is_circle) {
			stops = found_bus.route.size();
		}
		else {
			stops = found_bus.route.size() * 2 - 1;
			for (size_t i = found_bus.route.size() - 1; i > 0u; --i) {
				route_length += GetDistance(found_bus.route.at(i)->stop, found_bus.route.at(i - 1)->stop);
			}
			geo_length *= 2.0;
		}

		unique_stops = set(found_bus.route.begin(), found_bus.route.end()).size();

		return optional<BusInfo>({ found_bus.bus, stops, unique_stops, route_length, route_length / geo_length });
	}

	optional<set<string_view>> TransportCatalogue::GetBusesByStop(string_view stop) const {
		if (stops_by_name_.count(stop) == 0) {
			return nullopt;
		}
		return {buses_by_stop_.at(stops_by_name_.at(stop))};
	}

	void TransportCatalogue::SetDistance(string_view from, string_view to, size_t distance) {
		distances_[{ stops_by_name_.at(from), stops_by_name_.at(to) }] = distance;
	}

	size_t TransportCatalogue::GetDistance(string_view from, string_view to) const {
		if (distances_.count({ stops_by_name_.at(from), stops_by_name_.at(to) }) == 0) {
			return distances_.at({ stops_by_name_.at(to), stops_by_name_.at(from) });
		}
		return distances_.at({ stops_by_name_.at(from), stops_by_name_.at(to) });
	}

	const std::deque<Bus>& TransportCatalogue::GetAllBuses() const {
		return buses_;
	}

	const std::deque<Stop>& TransportCatalogue::GetAllStops() const {
		return stops_;
	}
}