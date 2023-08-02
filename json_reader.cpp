#include "json_reader.h"
#include "json_builder.h"
#include <sstream>
#include <algorithm>
#include <exception>

namespace transport_catalogue {

	namespace interfaces {

		/* Вспомогательные функции */
		namespace detail {

			bool CheckNodeType(const json::Node& node, std::string_view category) {
				return node.AsMap().at("type"s).AsString() == category;
			}

			domain::Stop BuildStopFromJSON(const json::Dict& json_stop) {
				return { json_stop.at("name"s).AsString(), { json_stop.at("latitude"s).AsDouble(), json_stop.at("longitude"s).AsDouble()} };
			}

			domain::Bus BuildBusFromJSON(const json::Dict& json_bus, const TransportCatalogue& catalogue) {
				json::Array stops = json_bus.at("stops"s).AsArray();
				std::vector<const domain::Stop*> route;
				route.reserve(stops.size());
				for (const json::Node& stop : stops) {
					route.push_back(catalogue.FindStopByName(stop.AsString()).value());
				}
				return { json_bus.at("name"s).AsString(), route, json_bus.at("is_roundtrip"s).AsBool() };
			}

			json::Dict TransformStopInfoToJSON(const std::optional<std::set<std::string_view>>& buses_by_stop, int id) { //rewrite with Builder
				if (!buses_by_stop) {
					return json::Builder{}.StartDict().
						Key("error_message"s).Value("not found"s).
						Key("request_id"s).Value(id).
						EndDict().Build().AsMap();
				}

				auto builder = json::Builder{};
				auto answer = builder.StartDict().Key("buses"s).StartArray();
				for (std::string_view bus : buses_by_stop.value()) {
					answer.Value(std::string(bus));
				}
				return answer.EndArray().Key("request_id"s).Value(id).EndDict().Build().AsMap();
			}

			json::Dict TransformBusInfoToJSON(const std::optional<domain::BusInfo>& bus_info, int id) { //rewrite with Builder
				if (!bus_info) {
					return json::Builder{}.StartDict().
						Key("error_message"s).Value("not found"s).
						Key("request_id"s).Value(id).
						EndDict().Build().AsMap();
				}

				return json::Builder{}.StartDict().Key("curvature"s).Value(bus_info.value().curve).
					Key("route_length"s).Value(static_cast<int>(bus_info.value().route_length)).
					Key("stop_count"s).Value(static_cast<int>(bus_info.value().stops)).
					Key("unique_stop_count"s).Value(static_cast<int>(bus_info.value().unique_stops)).
					Key("request_id"s).Value(id).EndDict().Build().AsMap();
			}

			svg::Color BuildColorFromJSON(const json::Node& json_color) {
				svg::Color color;

				if (json_color.IsString()) {
					color = { json_color.AsString() };
				}
				else {
					const json::Array color_array = json_color.AsArray();
					if (color_array.size() == 3) {
						color = svg::Rgb{
							static_cast<uint8_t>(color_array.at(0).AsInt()),
							static_cast<uint8_t>(color_array.at(1).AsInt()),
							static_cast<uint8_t>(color_array.at(2).AsInt())
						};
					}
					else {
						color = svg::Rgba{
							static_cast<uint8_t>(color_array.at(0).AsInt()),
							static_cast<uint8_t>(color_array.at(1).AsInt()),
							static_cast<uint8_t>(color_array.at(2).AsInt()),
							color_array.at(3).AsDouble()
						};
					}
				}
				return color;
			}

			json::Dict TransformMapToJSON(const std::string& map, int id) { //rewrite with Builder
				return json::Builder{}.StartDict().Key("map"s).Value(map).Key("request_id"s).Value(id).EndDict().Build().AsMap();
			}

			json::Dict TransformRouteInfoToJSON(const std::optional<TransportRouter::TransportRouteInfo>& route_info, int id) {
				if (!route_info) {
					return json::Builder{}.StartDict().
						Key("error_message"s).Value("not found"s).
						Key("request_id"s).Value(id).
						EndDict().Build().AsMap();
				}

				auto builder = json::Builder{};
				auto answer = builder.StartDict().Key("items"s).StartArray();
				for (auto segment : route_info->segments) {
					if (segment->type == RouteSegment::Type::WAIT) {
						answer.StartDict().
							Key("type"s).Value("Wait"s).
							Key("stop_name"s).Value(std::string(std::get<RouteSegment::WaitData>(segment->data))).
							Key("time"s).Value(segment->time).EndDict();
					}
					else {
						answer.StartDict().
							Key("type"s).Value("Bus"s).
							Key("bus"s).Value(std::string(std::get<RouteSegment::BusData>(segment->data).first)).
							Key("span_count"s).Value(std::get<RouteSegment::BusData>(segment->data).second).
							Key("time"s).Value(segment->time).EndDict();
					}
				}
				return answer.EndArray().
					Key("request_id"s).Value(id).
					Key("total_time"s).Value(route_info->weight).
					EndDict().Build().AsMap();
			}
		}

		/* JSONReader - PUBLIC */
		void JSONReader::LoadData(std::istream& input) {
			// 1) read the JSON
			json::Document doc = json::Load(input);

			// 2) add data to transport_catalogue
			if (doc.GetRoot().AsMap().count("base_requests"s) > 0) {
				json::Array query_queue = doc.GetRoot().AsMap().at("base_requests"s).AsArray();
				AddDataToCatalogueFromJSON(query_queue);
			}

			// 3) add queries to request_handler
			if (doc.GetRoot().AsMap().count("stat_requests"s) > 0) {
				AddRequestsToHandlerFromJSON(doc.GetRoot().AsMap().at("stat_requests"s).AsArray());
			}

			// 4) read setting for map_renderer
			if (doc.GetRoot().AsMap().count("render_settings"s) > 0) {
				AddSettingsToRendererFromJSON(doc.GetRoot().AsMap().at("render_settings"s).AsMap());
			}

			// 5) read setting for transport_router
			if (doc.GetRoot().AsMap().count("routing_settings"s) > 0) {
				AddSettingsAndBuildRouterFromJSON(doc.GetRoot().AsMap().at("routing_settings"s).AsMap());
			}

			// 6) add data to transport_catalogue
			if (doc.GetRoot().AsMap().count("serialization_settings"s) > 0) {
				AddSerializationFile(doc.GetRoot().AsMap().at("serialization_settings"s).AsMap());
			}
		}

		void JSONReader::PrintAnswers(std::ostream& output) {
			using namespace detail;
			auto builder = json::Builder{};
			auto answers = builder.StartArray();

			for (const RequestHandler::Query& query : handler_.GetRequests()) {
				json::Dict ans;
				if (query.type == RequestHandler::Query::Type::STOP) {
					auto opt = handler_.InfoStopRequest(query);
					ans = TransformStopInfoToJSON(opt, query.id);
				}
				else if (query.type == RequestHandler::Query::Type::BUS) {
					auto opt = handler_.InfoBusRequest(query);
					ans = TransformBusInfoToJSON(opt, query.id);
				}
				else if (query.type == RequestHandler::Query::Type::ROUTE) {
					auto opt = handler_.GetShortestRouteRequest(query);
					ans = TransformRouteInfoToJSON(opt, query.id);
				}
				else if (query.type == RequestHandler::Query::Type::MAP) {
					std::ostringstream stream;
					handler_.DrawMapRequest(stream);
					ans = TransformMapToJSON(stream.str(), query.id);
				}
				else {
					throw std::logic_error("Query type is unknown");
				}
				answers.Value(ans);
			}
			json::Print(json::Document{ answers.EndArray().Build() }, output);
		}

		std::optional<std::filesystem::path> JSONReader::GetSerializationFile() const {
			if (serialization_file_.has_filename()) {
				return serialization_file_;
			}
			return std::nullopt;
		}

		RequestHandler& JSONReader::GetHandler() {
			return handler_;
		}

		const RequestHandler& JSONReader::GetHandler() const {
			return handler_;
		}


		/* JSONReader - PRIVATE */
		void JSONReader::SetDistancesFromJSON(std::string_view from, const json::Dict& distances) {
			for (auto [to, json_distance] : distances) {
				catalogue_.SetDistance(from, to, static_cast<size_t>(json_distance.AsInt()));
			}
		}

		void JSONReader::AddDataToCatalogueFromJSON(json::Array& query_queue) {
			using namespace detail;
			std::sort(query_queue.begin(),
				query_queue.end(),
				[&](const json::Node& left, const json::Node& right) {
					return CheckNodeType(left, "Stop"sv) && CheckNodeType(right, "Bus"sv);
				});

			for (const json::Node& query : query_queue) {
				if (CheckNodeType(query, "Stop"sv)) {
					domain::Stop stop = BuildStopFromJSON(query.AsMap());
					catalogue_.AddStop(stop);
				}
				else {
					domain::Bus bus = BuildBusFromJSON(query.AsMap(), catalogue_);
					catalogue_.AddBus(bus);
				}
			}

			for (const auto& query : query_queue) {
				if (CheckNodeType(query, "Bus"sv)) {
					break;
				}
				else {
					SetDistancesFromJSON(query.AsMap().at("name"s).AsString(), query.AsMap().at("road_distances"s).AsMap());
				}
			}
		}

		void JSONReader::AddRequestsToHandlerFromJSON(const json::Array& query_queue) {
			using namespace detail;
			handler_.Reserve(query_queue.size());
			for (const json::Node& query : query_queue) {
				RequestHandler::Query::Type type;
				std::vector<std::string> parameters;

				if (CheckNodeType(query, "Stop"sv)) {
					type = RequestHandler::Query::Type::STOP;
					parameters.push_back(query.AsMap().at("name"s).AsString());
				}
				else if (CheckNodeType(query, "Bus"sv)) {
					type = RequestHandler::Query::Type::BUS;
					parameters.push_back(query.AsMap().at("name"s).AsString());
				}
				else if (CheckNodeType(query, "Route"sv)) {
					type = RequestHandler::Query::Type::ROUTE;
					parameters.push_back(query.AsMap().at("from"s).AsString());
					parameters.push_back(query.AsMap().at("to"s).AsString());
				}
				else if (CheckNodeType(query, "Map"sv)) {
					type = RequestHandler::Query::Type::MAP;
				}
				else {
					type = RequestHandler::Query::Type::UNDEFINED;
				}

				RequestHandler::Query formed_query{
					query.AsMap().at("id"s).AsInt(),
					type,
					std::move(parameters)
				};

				handler_.AddRequest(std::move(formed_query));
			}
		}

		void JSONReader::AddSettingsToRendererFromJSON(const json::Dict& json_settings) {
			using namespace detail;

			const json::Array& bus_label_offset_arr = json_settings.at("bus_label_offset"s).AsArray();
			svg::Point bus_label_offset{ bus_label_offset_arr.at(0).AsDouble(), bus_label_offset_arr.at(1).AsDouble() };

			const json::Array& stop_label_offset_arr = json_settings.at("stop_label_offset"s).AsArray();
			svg::Point stop_label_offset{ stop_label_offset_arr.at(0).AsDouble(), stop_label_offset_arr.at(1).AsDouble() };

			svg::Color underlayer_color = BuildColorFromJSON(json_settings.at("underlayer_color"s));

			const json::Array& pallete_arr = json_settings.at("color_palette"s).AsArray();
			std::vector<svg::Color> pallete;
			pallete.reserve(pallete_arr.size());

			for (const json::Node& json_color : pallete_arr) {
				pallete.push_back(BuildColorFromJSON(json_color));
			}

			handler_.SetRendererSettings(
				MapRenderer::Settings{
					json_settings.at("width"s).AsDouble(),
					json_settings.at("height"s).AsDouble(),
					json_settings.at("padding"s).AsDouble(),
					json_settings.at("line_width"s).AsDouble(),
					json_settings.at("stop_radius"s).AsDouble(),
					json_settings.at("bus_label_font_size"s).AsInt(),
					bus_label_offset,
					json_settings.at("stop_label_font_size"s).AsInt(),
					stop_label_offset,
					underlayer_color,
					json_settings.at("underlayer_width"s).AsDouble(),
					pallete
				}
			);
		}

		void JSONReader::AddSettingsAndBuildRouterFromJSON(const json::Dict& json_settings) {
			handler_.SetRouterSettings(
				TransportGraph::Settings{
					json_settings.at("bus_wait_time"s).AsInt(),
					json_settings.at("bus_velocity"s).AsDouble()
				}
			);
		}

		void JSONReader::AddSerializationFile(const json::Dict& serialization_settings) {
			serialization_file_ = serialization_settings.at("file"s).AsString();
		}
	}

}
