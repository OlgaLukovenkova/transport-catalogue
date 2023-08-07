#pragma once

#include "svg.h"
#include "domain.h"
#include <set>
#include <memory>
#include <cassert>
#include <algorithm>

namespace transport_catalogue {
	namespace interfaces {
		namespace detail {
			inline const double EPSILON = 1e-6;
			
			bool IsZero(double value);

			class SphereProjector {
			public:
				template <typename PointInputIt>
				SphereProjector(PointInputIt points_begin, PointInputIt points_end,
				    double max_width, double max_height, double padding)
				    : padding_(padding) 
				{
				    if (points_begin == points_end) {
					return;
				    }
				
				    const auto [left_it, right_it] = std::minmax_element(
					points_begin, points_end,
					[](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
				    min_lon_ = left_it->lng;
				    const double max_lon = right_it->lng;
				
				    const auto [bottom_it, top_it] = std::minmax_element(
					points_begin, points_end,
					[](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
				    const double min_lat = bottom_it->lat;
				    max_lat_ = top_it->lat;
				
				    std::optional<double> width_zoom;
				    if (!IsZero(max_lon - min_lon_)) {
					width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
				    }
				
				    std::optional<double> height_zoom;
				    if (!IsZero(max_lat_ - min_lat)) {
					height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
				    }
				
				    if (width_zoom && height_zoom) {
					zoom_coeff_ = std::min(*width_zoom, *height_zoom);
				    }
				    else if (width_zoom) {
					zoom_coeff_ = *width_zoom;
				    }
				    else if (height_zoom) {
					zoom_coeff_ = *height_zoom;
				    }
				}
				
				svg::Point operator()(geo::Coordinates coords) const;
			
			private:
				double padding_;
				double min_lon_ = 0;
				double max_lat_ = 0;
				double zoom_coeff_ = 0;
			};
        }

		class MapRenderer {
		public:
			struct Settings {
				double width;
				double height;
				double padding;
				double line_width;
				double stop_radius;
				int bus_label_font_size;
				svg::Point bus_label_offset;
				int stop_label_font_size;
				svg::Point stop_label_offset;
				svg::Color underlayer_color;
				double underlayer_width;
				std::vector<svg::Color>	color_palette;
			};
			
			MapRenderer() = default;

			template <typename T>
			void SetSettings(T&& settings) {
				settings_ = std::forward<T>(settings);
			}

           	const Settings& GetSettings() const;            
            
			void DrawMap(std::ostream& out, const std::set<domain::Bus>& buses) const;

		private:
			Settings settings_;

			detail::SphereProjector BuildProjector(const std::set<domain::Bus>& data) const;
			void AddBusToMap(const domain::Bus& bus, const detail::SphereProjector& projector, int& color_index, svg::Document& map_doc) const;       
			void AddBusNameToMap(const domain::Bus& bus, const detail::SphereProjector& projector, int& color_index, svg::Document& map_doc) const;
			std::set<domain::Stop> GetStopsFromBuses(const std::set<domain::Bus>& all_buses) const;
			void AddStopToMap(const domain::Stop& stop, const detail::SphereProjector& projector, svg::Document& map_doc) const;
			void AddStopNameToMap(const domain::Stop& stop, const detail::SphereProjector& projector, svg::Document& map_doc) const;
		};	
	}
}

