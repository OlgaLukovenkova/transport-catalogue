#pragma once
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"

#include <vector>
#include <string_view>
#include <iostream>

namespace transport_catalogue {

	constexpr double FACTOR_KM_PER_H_TO_M_PER_MIN = 1000.0 / 60.0;

	struct RouteSegment {

		enum class Type {
			WAIT,
			BUS
		};
		Type type;

		using WaitData = std::string_view;
		using BusData = std::pair<std::string_view, int>;
		std::variant<WaitData, BusData> data;
		double time;
	};

	class TransportGraph {
	public:
		struct Settings {
			int wait_time;
			double velocity;
		};
		
		template <typename T>
		TransportGraph(const TransportCatalogue& catalog, T&& settings)
			: settings_(std::forward<T>(settings)), graph_(catalog.GetAllStops().size() * 2) {
			AddStopsToGraph(catalog);
			AddBusesToGraph(catalog);
		}

		/* for serialization */
		template <typename Settings, typename Graph, typename Ids, typename Segments> 
		TransportGraph(Settings&& settings, Graph&& graph, Ids&& id_by_stops, Segments&& segments)
			: settings_(std::forward<Settings>(settings))
			, graph_(std::forward<Graph>(graph))
			, id_by_stops_(std::forward<Ids>(id_by_stops))
			, segments_(std::forward<Segments>(segments))
		{

		}

		const Settings& GetSettings() const {
			return settings_;
		}

		size_t GetSegmentCount() const {
			return segments_.size();
		}
		/* -------------------- */

		const graph::DirectedWeightedGraph<double>& GetInnerGraph() const;

		size_t GetAfterWaitingStopID(std::string_view stop_name) const;  // id * 2 + 1
		size_t GetBeforeWaitingStopID(std::string_view stop_name) const; // id * 2
		const RouteSegment& GetSegmentByID(size_t id) const;

	private:
		Settings settings_;
		graph::DirectedWeightedGraph<double> graph_;
		std::unordered_map<std::string_view, size_t> id_by_stops_ = {};
		std::vector<RouteSegment> segments_ = {};

		static size_t GetNthOdd(size_t stop_id_from_stops_);
		static size_t GetNthEven(size_t stop_id_from_stops_);

		void AddStopsToGraph(const TransportCatalogue& catalog);

		template <typename It>
		void AddBusRoute(const TransportCatalogue& catalog, std::string_view bus, It first, It last) {
			size_t stops_count = last - first;
			segments_.resize(segments_.size() + (stops_count - 1) * stops_count / 2);

			for (auto from = first; from < last - 1; ++from) {
				std::string_view stop_from_name = (*from)->stop;
				size_t stop_from_id = id_by_stops_[stop_from_name];

				std::string_view last_stop_name = stop_from_name;
				double distance = 0.0;

				for (auto to = from + 1; to < last; ++to) {
					std::string_view stop_to_name = (*to)->stop;
					size_t stop_to_id = id_by_stops_[stop_to_name];

					distance += catalog.GetDistance(last_stop_name, stop_to_name);
					last_stop_name = stop_to_name;
					double time = distance / (settings_.velocity * FACTOR_KM_PER_H_TO_M_PER_MIN);

					size_t segment_id = graph_.AddEdge({ TransportGraph::GetNthOdd(stop_from_id),
						TransportGraph::GetNthEven(stop_to_id),
						time });
					segments_[segment_id] = { RouteSegment::Type::BUS,
						std::make_pair(bus, to - from), time };
				}
			}
		}
		void AddBusesToGraph(const TransportCatalogue& catalog);
	};

	class TransportRouter
	{
	public:
		struct TransportRouteInfo {
			double weight;
			std::vector<const RouteSegment*> segments;
		};

		template <typename T>
		TransportRouter(const TransportCatalogue& catalog, T&& settings)
			: graph_(catalog, std::forward<T>(settings)), router_(graph_.GetInnerGraph()) {

		}

		/* for serialization */
		template <typename TransportGraph, typename RouterInternalData>
		TransportRouter(TransportGraph&& graph, RouterInternalData&& router_data)
			: graph_(std::forward<TransportGraph>(graph)), router_(graph_.GetInnerGraph(), std::forward<RouterInternalData>(router_data))
		{

		}

		const TransportGraph& GetTransportGraph() const {
			return graph_;
		}

		const graph::Router<double>& GetInnerRouter() const {
			return router_;
		}
		/* ----------------- */

		std::optional<TransportRouteInfo> GetShortestRoute(std::string_view from, std::string_view to) const;
		


	private:
		TransportGraph graph_;
		graph::Router<double> router_;
	};
}

