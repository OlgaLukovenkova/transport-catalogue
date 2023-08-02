#include "serialization.h"
#include <fstream>

namespace transport_catalogue {

	namespace interfaces {

		namespace detail {
			struct ColorSerializator {
				ProtoColor& proto_color;

				void operator()(std::monostate) {
					proto_color.mutable_string_color()->set_color("none"s);
				}
				void operator()(const std::string& color) {
					proto_color.mutable_string_color()->set_color(color);
				}
				void operator()(const svg::Rgb& color) {
					uint32_t rgb = color.red << 16 | color.green << 8 | color.blue;
					proto_color.mutable_rgb_color()->set_rgb(rgb);
				}
				void operator()(const svg::Rgba& color) {
					uint32_t rgb = color.red << 16 | color.green << 8 | color.blue;
					proto_color.mutable_rgba_color()->set_rgb(rgb);
					proto_color.mutable_rgba_color()->set_alpha(color.opacity);
				}
			};
		}

		/*------Serializator------*/

		Serializator::Serializator(const TransportCatalogue& catalog, const RequestHandler& handler, const std::filesystem::path& path)
			: catalog_(catalog)
			, map_settings_(handler.GetRendererSettings())
			, router_(handler.GetRouter())
			, file_(path) 
		{

		}

		bool Serializator::Serialize() {
			std::ofstream out(file_, std::ios::binary);

			if (!out) {
				return false;
			}

			::transport_catalogue_serialize::AllContent proto_content;
			*proto_content.mutable_catalog() = SerializeCatalogue();
			*proto_content.mutable_map_settings() = SerializeMapSettings();
			*proto_content.mutable_transport_router() = SerializeTransportRouter();
			stop_table_.clear();
			bus_table_.clear();
			
			return proto_content.SerializeToOstream(&out);
		}

		/* TransportCatalogue */
		ProtoStop Serializator::SerializeStop(const Stop& stop) {
			ProtoStop proto_stop;
			proto_stop.set_stop(stop.stop);
			proto_stop.mutable_coords()->set_lat(stop.coords.lat);
			proto_stop.mutable_coords()->set_lng(stop.coords.lng);
			stop_table_[stop.stop] = static_cast<uint32_t>(stop_table_.size());
			return proto_stop;
		}

		ProtoBus Serializator::SerializeBus(const Bus& bus) {
			ProtoBus proto_bus;
			proto_bus.set_bus(bus.bus);
			proto_bus.set_is_circle(bus.is_circle);

			for (const Stop* stop : bus.route) {
				uint32_t number = stop_table_.at(stop->stop);
				proto_bus.add_route(number);
			}
			bus_table_[bus.bus] = static_cast<uint32_t>(bus_table_.size());
			return proto_bus;

		}

		ProtoDistance Serializator::SerializeDistance(const Stop& from, const Stop& to, size_t distance) const {
			ProtoDistance proto_distance;
			proto_distance.set_from_stop(stop_table_.at(from.stop));
			proto_distance.set_to_stop(stop_table_.at(to.stop));
			proto_distance.set_distance(distance);
			return proto_distance;
		}

		ProtoTransportCatalogue Serializator::SerializeCatalogue() {
			ProtoTransportCatalogue proto_catalog;
			size_t number = stop_table_.size();
			for (const Stop& stop : catalog_.GetAllStops()) {
				*proto_catalog.add_stops() = SerializeStop(stop);
			}

			for (const Bus& bus : catalog_.GetAllBuses()) {
				*proto_catalog.add_buses() = SerializeBus(bus);
			}

			for (const auto& [stop_pair, distance] : catalog_.GetAllDistances()) {
				*proto_catalog.add_distances() = SerializeDistance(*stop_pair.first, *stop_pair.second, distance);
			}

			return proto_catalog;
		}

		/* MapRenderer::Settings */
		ProtoColor Serializator::SerializeColor(const svg::Color& color) const {
			ProtoColor proto_color;
			std::visit(detail::ColorSerializator{ proto_color }, color);
			return proto_color;
		}

		ProtoPoint Serializator::SerializePoint(const svg::Point& point) const {
			ProtoPoint proto_point;
			proto_point.set_x(point.x);
			proto_point.set_y(point.y);
			return proto_point;
		}

		ProtoMapRendererSettings Serializator::SerializeMapSettings() const {
			ProtoMapRendererSettings proto_settings;

			proto_settings.set_width(map_settings_.width);
			proto_settings.set_height(map_settings_.height);
			proto_settings.set_padding(map_settings_.padding);
			proto_settings.set_line_width(map_settings_.line_width);
			proto_settings.set_stop_radius(map_settings_.stop_radius);
			proto_settings.set_bus_label_font_size(map_settings_.bus_label_font_size);
			*proto_settings.mutable_bus_label_offset() = SerializePoint(map_settings_.bus_label_offset);
			proto_settings.set_stop_label_font_size(map_settings_.stop_label_font_size);
			*proto_settings.mutable_stop_label_offset() = SerializePoint(map_settings_.stop_label_offset);
			*proto_settings.mutable_underlayer_color() = SerializeColor(map_settings_.underlayer_color);
			proto_settings.set_underlayer_width(map_settings_.underlayer_width);
			for (const svg::Color& color : map_settings_.color_palette) {
				*proto_settings.add_color_palette() = SerializeColor(color);
			}

			return proto_settings;
		}

		/* TransportRouter */
		ProtoGraph Serializator::SerializeInnerGraph() const {
			ProtoGraph proto_graph;
			const Graph& graph = router_.GetTransportGraph().GetInnerGraph();

			for (int edge_id = 0; edge_id < graph.GetEdgeCount(); ++edge_id) {
				const auto& edge = graph.GetEdge(edge_id);
				::graph_serialize::Edge proto_edge;
				proto_edge.set_from(edge.from);
				proto_edge.set_to(edge.to);
				proto_edge.set_weight(edge.weight);
				*proto_graph.add_edges() = std::move(proto_edge);
			}

			return proto_graph;
		}

		ProtoTransportRouterSettings Serializator::SerializeRouterSettings() const {
			ProtoTransportRouterSettings proto_settings;
			const auto& settings = router_.GetTransportGraph().GetSettings();
			proto_settings.set_velocity(settings.velocity);
			proto_settings.set_wait_time(settings.wait_time);
			return proto_settings;
		}

		ProtoRouteSegment Serializator::SerializeRouteSegment(const RouteSegment& segment) const {
			ProtoRouteSegment proto_segment;
			proto_segment.set_time(segment.time);
			switch (segment.type) {
			case RouteSegment::Type::BUS:
			{
				proto_segment.set_type(::transport_router_serialize::RouteSegment_Type_BUS);
				::transport_router_serialize::BusData proto_bus_data;
				proto_bus_data.set_bus_id(bus_table_.at(std::get<RouteSegment::BusData>(segment.data).first));
				proto_bus_data.set_stop_count(std::get<RouteSegment::BusData>(segment.data).second);
				*proto_segment.mutable_bus_data() = std::move(proto_bus_data);
				break;
			}
			case RouteSegment::Type::WAIT:
			{
				proto_segment.set_type(transport_router_serialize::RouteSegment_Type_WAIT);
				::transport_router_serialize::WaitData proto_wait_data;
				proto_wait_data.set_stop_id(stop_table_.at(std::get<RouteSegment::WaitData>(segment.data)));
				*proto_segment.mutable_wait_data() = std::move(proto_wait_data);
				break;
			}
			}
			return proto_segment;
		}

		ProtoTransportGraph Serializator::SerializeTransportGraph() const {
			ProtoTransportGraph proto_graph;
			const auto& graph_ = router_.GetTransportGraph();

			*proto_graph.mutable_graph() = SerializeInnerGraph();
			*proto_graph.mutable_settings() = SerializeRouterSettings();

			for (const Stop& stop : catalog_.GetAllStops()) {
				proto_graph.mutable_id_by_stops_()->insert({stop_table_.at(stop.stop), graph_.GetBeforeWaitingStopID(stop.stop) / 2 });				
			}

			for (int seg = 0; seg < graph_.GetSegmentCount(); ++seg) {
				*proto_graph.add_segments() = SerializeRouteSegment(graph_.GetSegmentByID(seg));
			}
			return proto_graph;
		}

		ProtoOptionalRouteInternalData Serializator::SerializeOptionalRouteInternalData(const std::optional<RouteInternalData>& data) const {
			ProtoOptionalRouteInternalData optional_proto_data;
			if (data) {
				optional_proto_data.mutable_data()->set_weight(data->weight);
				if (data->prev_edge) {
					optional_proto_data.mutable_data()->mutable_prev_edge()->set_edge_id(*(data->prev_edge));
				}
			}
			return optional_proto_data;
		}

		ProtoRouter Serializator::SerializeInnerRouter() const {
			const Router& router = router_.GetInnerRouter();
			ProtoRouter proto_inner_router;

			for (const std::vector<std::optional<RouteInternalData>>& line : router.GetRoutesInternalData()) {
				auto ptr_routes_internal_data_line = proto_inner_router.add_routes_internal_data();
				for (const std::optional<graph::Router<double>::RouteInternalData>& data : line) {
					*ptr_routes_internal_data_line->add_items() = SerializeOptionalRouteInternalData(data);
				}
			}
			return proto_inner_router;
		}

		ProtoTransportRouter Serializator::SerializeTransportRouter() const {
			ProtoTransportRouter proto_router;
			*proto_router.mutable_transport_graph() = SerializeTransportGraph();
			*proto_router.mutable_router() = SerializeInnerRouter();			
			return proto_router;
		}

		/* ------ Deserializator ------ */

		/* TransportCatalogue */
		Deserializator::Deserializator(TransportCatalogue& catalog, RequestHandler& handler, const std::filesystem::path& path)
			: catalog_(catalog)
			, handler_(handler)
			, file_(path) 
		{

		}

		bool Deserializator::Deserialize() {
			std::ifstream in(file_, std::ios::binary);
			
			if (!in) {
				return false;
			}

			if (!proto_content_.ParseFromIstream(&in)) {
				return false;
			}

			if (proto_content_.has_catalog()) {
				DeserializeCatalogue();
			}

			if (proto_content_.has_map_settings()) {
				DeserializeMapSettings();
			}

			if (proto_content_.has_transport_router()) {
				DeserializeTransportRouter();
			}

			return true;
		}

		Stop Deserializator::DeserializeStop(const ProtoStop& proto_stop) const {
			return Stop{ proto_stop.stop(), { proto_stop.coords().lat(), proto_stop.coords().lng() } };
		}

		Bus Deserializator::DeserializeBus(const ProtoBus& proto_bus) const {
			Bus bus{ proto_bus.bus(), {}, proto_bus.is_circle() };
			bus.route.reserve(proto_bus.route_size());
			for (int i = 0; i < proto_bus.route_size(); ++i) {
				uint32_t stop_number = proto_bus.route(i);
				std::string stop_name = proto_content_.catalog().stops(stop_number).stop();
				bus.route.emplace_back(*catalog_.FindStopByName(stop_name));
			}
			return bus;
		}

		void Deserializator::DeserializeDistances() {
			for (int i = 0; i < proto_content_.catalog().distances_size(); ++i) {
				const ProtoDistance& proto_distance = proto_content_.catalog().distances(i);
				std::string from = proto_content_.catalog().stops(proto_distance.from_stop()).stop();
				std::string to = proto_content_.catalog().stops(proto_distance.to_stop()).stop();
				catalog_.SetDistance(from, to, proto_distance.distance());
			}
		}

		void Deserializator::DeserializeCatalogue() {
			for (int i = 0; i < proto_content_.catalog().stops_size(); ++i) {
				catalog_.AddStop(DeserializeStop(proto_content_.catalog().stops(i)));
			}

			for (int i = 0; i < proto_content_.catalog().buses_size(); ++i) {
				catalog_.AddBus(DeserializeBus(proto_content_.catalog().buses(i)));
			}
			DeserializeDistances();
		}


		/*MapRenderer::Settings*/
		svg::Point Deserializator::DeserializePoint(const ProtoPoint& proto_point) const {
			return { proto_point.x(), proto_point.y() };
		}

		svg::Color Deserializator::DeserializeColor(const ProtoColor& proto_color) const {
			if (proto_color.has_string_color()) {
				if (proto_color.string_color().color() == "none"s) {
					return {};
				}
				else {
					return proto_color.string_color().color();
				}
			}

			if (proto_color.has_rgb_color()) {
				uint32_t rgb_code = proto_color.rgb_color().rgb();
				return svg::Rgb{
					static_cast<uint8_t>((rgb_code & 0x00FF0000) >> 16),
						static_cast<uint8_t>((rgb_code & 0x0000FF00) >> 8),
							static_cast<uint8_t>(rgb_code & 0x000000FF)
				};
			}

			if (proto_color.has_rgba_color()) {
				uint32_t rgb_code = proto_color.rgba_color().rgb();
				return svg::Rgba{
					static_cast<uint8_t>((rgb_code & 0x00FF0000) >> 16),
						static_cast<uint8_t>((rgb_code & 0x0000FF00) >> 8),
						static_cast<uint8_t>(rgb_code & 0x000000FF),
					proto_color.rgba_color().alpha()
				};
			}

			return {};
		}

		void Deserializator::DeserializeMapSettings() {
			MapRenderer::Settings settings;
			const auto& proto_map_settings_ = proto_content_.map_settings();
			settings.width = proto_map_settings_.width();
			settings.height = proto_map_settings_.height();
			settings.padding = proto_map_settings_.padding();
			settings.line_width = proto_map_settings_.line_width();
			settings.stop_radius = proto_map_settings_.stop_radius();
			settings.bus_label_font_size = proto_map_settings_.bus_label_font_size();
			settings.bus_label_offset = DeserializePoint(proto_map_settings_.bus_label_offset());
			settings.stop_label_font_size = proto_map_settings_.stop_label_font_size();
			settings.stop_label_offset = DeserializePoint(proto_map_settings_.stop_label_offset());
			settings.underlayer_color = DeserializeColor(proto_map_settings_.underlayer_color());
			settings.underlayer_width = proto_map_settings_.underlayer_width();
			settings.color_palette.reserve(proto_map_settings_.color_palette_size());
			for (int i = 0; i < proto_map_settings_.color_palette_size(); ++i) {
				settings.color_palette.emplace_back(DeserializeColor(proto_map_settings_.color_palette(i)));
			}
			handler_.SetRendererSettings(std::move(settings));
		}

		TransportGraph::Settings Deserializator::DeserializeRouterSettings() const {
			const auto& proto_settings = proto_content_.transport_router().transport_graph().settings();
			return { proto_settings.wait_time(), proto_settings.velocity() };
		}

		Graph Deserializator::DeserializeInnerGraph() const {
			const auto& proto_graph = proto_content_.transport_router().transport_graph().graph();
			graph::DirectedWeightedGraph<double> graph(catalog_.GetAllStops().size() * 2);
			
			for (int edge_id = 0; edge_id < proto_graph.edges_size(); ++edge_id) {
				graph::Edge<double> edge{ proto_graph.edges(edge_id).from(),
					proto_graph.edges(edge_id).to(),
					proto_graph.edges(edge_id).weight() };
				graph.AddEdge(edge);				
			}

			return graph;
		}

		RouteSegment Deserializator::DeserializeRouteSegment(const ProtoRouteSegment& proto_segment) const {
			RouteSegment segment;
			segment.time = proto_segment.time();

			switch (proto_segment.type()) {
			case ::transport_router_serialize::RouteSegment_Type_BUS:
			{
				segment.type = RouteSegment::Type::BUS;
				segment.data = RouteSegment::BusData{
					proto_content_.catalog().buses(proto_segment.bus_data().bus_id()).bus(),
					proto_segment.bus_data().stop_count()
				};
				break;
			}
			case ::transport_router_serialize::RouteSegment_Type_WAIT:
			{
				segment.type = RouteSegment::Type::WAIT;
				segment.data = RouteSegment::WaitData{ proto_content_.catalog().stops(proto_segment.wait_data().stop_id()).stop() };
				break;
			}
			}

			return segment;
		}

		TransportGraph Deserializator::DeserializeTransportGraph() const {
			std::unordered_map<std::string_view, size_t> id_by_stops;
			const auto& proto_graph = proto_content_.transport_router().transport_graph();
			for (const auto& item : proto_graph.id_by_stops_()) {
				id_by_stops[proto_content_.catalog().stops(item.first).stop()] = item.second;
			}

			std::vector<RouteSegment> segments;
			segments.reserve(proto_graph.segments_size());
			for (int seg = 0; seg < proto_graph.segments_size(); ++seg) {
				segments.emplace_back(DeserializeRouteSegment(proto_graph.segments(seg)));
			}

			return { DeserializeRouterSettings(),
				DeserializeInnerGraph(),
				std::move(id_by_stops),
				std::move(segments) };
		}

		std::optional<RouteInternalData> Deserializator::DeserializeOptionalRouteInternalData(const ProtoOptionalRouteInternalData& optional_proto_data) const {
			if (optional_proto_data.has_data()) {
				graph::Router<double>::RouteInternalData data;
				data.weight = optional_proto_data.data().weight();
				if (optional_proto_data.data().has_prev_edge()) {
					data.prev_edge = optional_proto_data.data().prev_edge().edge_id();
				}
				return data;
			}
			else {
				return std::nullopt;
			}
		}

		Router::RoutesInternalData Deserializator::DeserializeInnerRouterData() const {
			Router::RoutesInternalData routes_internal_data;	
			const auto& proto_router_data = proto_content_.transport_router().router().routes_internal_data();

			routes_internal_data.reserve(proto_router_data.size());

			for (int line = 0; line < proto_router_data.size(); ++line) {
				routes_internal_data.emplace_back();
				const auto& proto_router_data_line = proto_router_data.at(line);
				for (int index = 0; index < proto_router_data_line.items_size(); ++index) {
					routes_internal_data[line].emplace_back(DeserializeOptionalRouteInternalData(proto_router_data_line.items(index)));
				}
			}

			return routes_internal_data;
		}

		void Deserializator::DeserializeTransportRouter() {
			handler_.SetRouter(DeserializeTransportGraph(), DeserializeInnerRouterData());
		}
	}
}
