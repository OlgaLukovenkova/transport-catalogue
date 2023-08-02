#pragma once
#include "transport_catalogue.h"
#include "request_handler.h"

#include <transport_catalogue.pb.h>
#include <graph.pb.h>
#include <filesystem>


namespace transport_catalogue {

	namespace interfaces {
		using NumberTable = std::unordered_map<std::string_view, uint32_t>;
		using ProtoStop = ::transport_catalogue_serialize::Stop;
		using ProtoBus = ::transport_catalogue_serialize::Bus;
		using ProtoDistance = ::transport_catalogue_serialize::Distance;
		using ProtoTransportCatalogue = ::transport_catalogue_serialize::TransportCatalogue;

		using ProtoColor = ::svg_serialize::Color;
		using ProtoPoint = ::svg_serialize::Point;
		using ProtoMapRendererSettings = ::map_renderer_serialize::Settings;

		using ProtoGraph = ::graph_serialize::Graph;
		using ProtoRouter = ::graph_serialize::Router;
		using ProtoOptionalRouteInternalData = ::graph_serialize::OptionalRouteInternalData;
		using ProtoTransportRouter = ::transport_router_serialize::TransportRouter;
		using ProtoTransportRouterSettings = ::transport_router_serialize::Settings;
		using ProtoTransportGraph = ::transport_router_serialize::TransportGraph;
		using ProtoRouteSegment = ::transport_router_serialize::RouteSegment;

		using Graph = graph::DirectedWeightedGraph<double>;
		using Router = graph::Router<double>;
		using RouteInternalData = Router::RouteInternalData;

		class Serializator {
		public:
			Serializator(const TransportCatalogue& catalog, const RequestHandler& handler, const std::filesystem::path& path);
			bool Serialize();
		private:

			const TransportCatalogue& catalog_;
			const MapRenderer::Settings& map_settings_;
			const TransportRouter& router_;
			std::filesystem::path file_;
			NumberTable stop_table_;
			NumberTable bus_table_;

			ProtoStop SerializeStop(const Stop& stop);
			ProtoBus SerializeBus(const Bus& bus);
			ProtoDistance SerializeDistance(const Stop& from, const Stop& to, size_t distance) const;
			ProtoTransportCatalogue SerializeCatalogue();
			
			ProtoColor SerializeColor(const svg::Color& color) const; 
			ProtoPoint SerializePoint(const svg::Point& point) const;
			ProtoMapRendererSettings SerializeMapSettings() const;
			
			ProtoGraph SerializeInnerGraph() const;
			ProtoTransportRouterSettings SerializeRouterSettings() const;
			ProtoRouteSegment SerializeRouteSegment(const RouteSegment& segment) const;
			ProtoTransportGraph SerializeTransportGraph() const;
			ProtoRouter SerializeInnerRouter() const;
			ProtoOptionalRouteInternalData SerializeOptionalRouteInternalData(const std::optional<RouteInternalData>& data) const;
			ProtoTransportRouter SerializeTransportRouter() const;
			
		};

		class Deserializator {
		public:
			Deserializator(TransportCatalogue& catalog, RequestHandler& handler, const std::filesystem::path& path);
			bool Deserialize();

		private:
			TransportCatalogue& catalog_;
			RequestHandler& handler_;
			std::filesystem::path file_;
			::transport_catalogue_serialize::AllContent proto_content_;

			Stop DeserializeStop(const ProtoStop& proto_stop) const;
			Bus DeserializeBus(const ProtoBus& proto_bus) const;
			void DeserializeDistances();
			void DeserializeCatalogue();

			svg::Point DeserializePoint(const ProtoPoint& proto_point) const;
			svg::Color DeserializeColor(const ProtoColor& proto_color) const;
			void DeserializeMapSettings();

			TransportGraph::Settings DeserializeRouterSettings() const;
			Graph DeserializeInnerGraph() const;
			RouteSegment DeserializeRouteSegment(const ProtoRouteSegment& proto_segment) const;
			TransportGraph DeserializeTransportGraph() const;
			Router::RoutesInternalData DeserializeInnerRouterData() const;
			std::optional<RouteInternalData> DeserializeOptionalRouteInternalData(const ProtoOptionalRouteInternalData& optional_proto_data) const;
			void DeserializeTransportRouter();
		};
	}
}

