#include "map_renderer.h"

namespace transport_catalogue {

	namespace interfaces {
		
		namespace detail {
			bool IsZero(double value) {
				return std::abs(value) < EPSILON;
			}

			svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
				return {
						(coords.lng - min_lon_) * zoom_coeff_ + padding_,
						(max_lat_ - coords.lat) * zoom_coeff_ + padding_
				};
			}
		}

		// --------- MapRenderer PUBLIC -------------------
        const MapRenderer::Settings& MapRenderer::GetSettings() const {
            return settings_;
        }

        void MapRenderer::DrawMap(std::ostream& out, const std::set<domain::Bus>& buses) const {

            //1) define scaling coefficients
            detail::SphereProjector projector = BuildProjector(buses);

            //2) put objects on map, layer by layer
            svg::Document map_doc;
            // I 
            int color_index = 0;
            std::for_each(buses.begin(), buses.end(), [&](const auto& bus) { AddBusToMap(bus, projector, color_index, map_doc); });
            // II 
            color_index = 0;
            std::for_each(buses.begin(), buses.end(), [&](const auto& bus) { AddBusNameToMap(bus, projector, color_index, map_doc); });
            // III 
            auto stops = GetStopsFromBuses(buses);
            std::for_each(stops.begin(), stops.end(), [&](const auto& stop) { AddStopToMap(stop, projector, map_doc); });
            // IV 
            std::for_each(stops.begin(), stops.end(), [&](const auto& stop) { AddStopNameToMap(stop, projector, map_doc); });

            //3) draw
            map_doc.Render(out);
        }

        // --------- MapRenderer PRIVATE -------------------
        detail::SphereProjector MapRenderer::BuildProjector(const std::set<domain::Bus>& data) const {
            std::vector<geo::Coordinates> coords;

            for (const domain::Bus& bus : data) {
                for (const domain::Stop* stop : bus.route) {
                    coords.push_back(stop->coords);
                }
            }

            return { coords.begin(), coords.end(), settings_.width, settings_.height, settings_.padding };
        }

        void MapRenderer::AddBusToMap(const domain::Bus& bus, const detail::SphereProjector& projector, int& color_index, svg::Document& map_doc) const {
            if (bus.route.empty()) {
                return;
            }

            svg::Polyline route;

            for (const domain::Stop* stop : bus.route) {
                route.AddPoint(projector(stop->coords));
            }

            if (!bus.is_circle) {
                for (auto it = bus.route.rbegin() + 1; it != bus.route.rend(); ++it) {
                    route.AddPoint(projector((*it)->coords));
                }
            }

            route.SetStrokeColor(settings_.color_palette.at(color_index)).SetStrokeWidth(settings_.line_width);
            route.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            route.SetFillColor(svg::NoneColor);

            map_doc.Add(route);
            color_index = (color_index + 1) % static_cast<int>(settings_.color_palette.size());
        }

        void MapRenderer::AddBusNameToMap(const domain::Bus& bus, const detail::SphereProjector& projector, int& color_index, svg::Document& map_doc) const {
            if (bus.route.empty()) {
                return;
            }

            svg::Text under_text;
            under_text.SetData(bus.bus).SetFontFamily("Verdana").SetFontWeight("bold").SetFontSize(settings_.bus_label_font_size);
            under_text.SetOffset(settings_.bus_label_offset).SetPosition(projector(bus.route[0]->coords));

            svg::Text text(under_text);
            text.SetFillColor(settings_.color_palette.at(color_index));

            under_text.SetFillColor(settings_.underlayer_color).SetStrokeColor(settings_.underlayer_color);
            under_text.SetStrokeWidth(settings_.underlayer_width);
            under_text.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            map_doc.Add(under_text);
            map_doc.Add(text);

            if (!bus.is_circle && bus.route.back() != bus.route.front()) {
                under_text.SetPosition(projector(bus.route.back()->coords));
                text.SetPosition(projector(bus.route.back()->coords));
                map_doc.Add(under_text);
                map_doc.Add(text);
            }

            color_index = (color_index + 1) % static_cast<int>(settings_.color_palette.size());
        }

        std::set<domain::Stop> MapRenderer::GetStopsFromBuses(const std::set<domain::Bus>& all_buses) const {
            std::set<domain::Stop> stops;

            for (const auto& bus : all_buses) {
                for (const domain::Stop* stop : bus.route) {
                    stops.insert(*stop);
                }
            }

            return stops;
        }

        void MapRenderer::AddStopToMap(const domain::Stop& stop, const detail::SphereProjector& projector, svg::Document& map_doc) const {
            svg::Circle circle;
            circle.SetRadius(settings_.stop_radius).SetFillColor("white").SetCenter(projector(stop.coords));
            map_doc.Add(circle);
        }

        void MapRenderer::AddStopNameToMap(const domain::Stop& stop, const detail::SphereProjector& projector, svg::Document& map_doc) const {
            svg::Text under_text;
            under_text.SetData(stop.stop).SetFontFamily("Verdana").SetFontSize(settings_.stop_label_font_size);
            under_text.SetOffset(settings_.stop_label_offset).SetPosition(projector(stop.coords));

            svg::Text text(under_text);
            text.SetFillColor("black");

            under_text.SetFillColor(settings_.underlayer_color).SetStrokeColor(settings_.underlayer_color);
            under_text.SetStrokeWidth(settings_.underlayer_width);
            under_text.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            map_doc.Add(under_text);
            map_doc.Add(text);

        }

	}

}