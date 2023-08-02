#include "transport_router.h"

namespace transport_catalogue {

	/*------ TransportGraph ------*/
	const graph::DirectedWeightedGraph<double>& TransportGraph::GetInnerGraph() const {
		return graph_;
	}

	size_t TransportGraph::GetAfterWaitingStopID(std::string_view stop_name) const {
		return TransportGraph::GetNthOdd(id_by_stops_.at(stop_name));
	}

	size_t TransportGraph::GetBeforeWaitingStopID(std::string_view stop_name) const {
		return TransportGraph::GetNthEven(id_by_stops_.at(stop_name));
	}

	const RouteSegment& TransportGraph::GetSegmentByID(size_t id) const {
		return segments_.at(id);
	}

	size_t TransportGraph::GetNthOdd(size_t stop_id_from_stops_) {
		return stop_id_from_stops_ * 2 + 1;
	}

	size_t TransportGraph::GetNthEven(size_t stop_id_from_stops_) {
		return stop_id_from_stops_ * 2;
	}

	void TransportGraph::AddStopsToGraph(const TransportCatalogue& catalog) {
		const std::deque<Stop>& stops = catalog.GetAllStops();
		//stops_.reserve(stops.size() * 2);
		segments_.resize(stops.size());
		for (const Stop& stop_info : stops) {
			//stops_.push_back(stop_info.stop);

			size_t id = id_by_stops_.size();
			id_by_stops_[stop_info.stop] = id;

			size_t segment_id = graph_.AddEdge({ TransportGraph::GetNthEven(id),
				TransportGraph::GetNthOdd(id),
				static_cast<double>(settings_.wait_time) });
			segments_[segment_id] = { RouteSegment::Type::WAIT,
				stop_info.stop, static_cast<double>(settings_.wait_time) };
		}
	}

	void TransportGraph::AddBusesToGraph(const TransportCatalogue& catalog) {
		const std::deque<Bus>& buses = catalog.GetAllBuses();
		for (const Bus& bus : buses) {
			AddBusRoute(catalog, bus.bus, bus.route.begin(), bus.route.end());
			if (!bus.is_circle) {
				AddBusRoute(catalog, bus.bus, bus.route.rbegin(), bus.route.rend());
			}
		}
	}

	/* ------ TransportRouter ------ */
	std::optional<TransportRouter::TransportRouteInfo> TransportRouter::GetShortestRoute(std::string_view from, std::string_view to) const {
		size_t from_id = graph_.GetBeforeWaitingStopID(from);
		size_t to_id = graph_.GetBeforeWaitingStopID(to);

		auto answer = router_.BuildRoute(from_id, to_id);

		if (!answer) {
			return std::nullopt;
		}

		std::vector<const RouteSegment*> result;
		result.reserve(answer->edges.size());
		for (auto edge_id : answer->edges) {
			result.push_back(&graph_.GetSegmentByID(edge_id));
		}
		return TransportRouteInfo{ answer->weight, result };
	}
}
