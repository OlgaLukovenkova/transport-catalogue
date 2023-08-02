#pragma once
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

namespace transport_catalogue {

	namespace interfaces {

		using namespace std::literals;

		/* Фасад для TransportCatalogue, MapRenderer, TransportRouter*/
		class RequestHandler
		{
		public:

			struct Query {
				enum class Type { STOP, BUS, MAP, ROUTE, UNDEFINED };
				int id;
				Type type;
				std::vector<std::string> parameters;
			};

		public:
			RequestHandler(const TransportCatalogue& catalogue) 
				: catalogue_(catalogue), 
				requests_(), 
				renderer_(nullptr), 
				router_(nullptr) {
			};

			void Reserve(size_t capacity);
			void AddRequest(const Query& request);
			
			template <typename Settings>
			void SetRendererSettings(Settings&& settings) {
				renderer_ = std::make_unique<MapRenderer>();
				renderer_->SetSettings(std::forward<Settings>(settings));
			}

			const MapRenderer::Settings& GetRendererSettings() const;
			
			template <typename Settings>
			void SetRouterSettings(Settings&& settings) {
				router_ = std::make_unique<TransportRouter>(catalogue_, std::forward<Settings>(settings));
			}

			/* for serialization */
			template <typename Graph, typename RouterInternalData>
			void SetRouter(Graph&& graph, RouterInternalData&& router_data) {
				router_ = std::make_unique<TransportRouter>(std::forward<Graph>(graph), std::forward<RouterInternalData>(router_data));
			}

			const TransportRouter& GetRouter() const {
				if (!router_) {
					throw std::logic_error("The router was not created");
				}
				return *router_;
			}
			/* ----------------- */

			const std::vector<Query>& GetRequests() const;
			std::optional<std::set<std::string_view>> InfoStopRequest(const Query& query) const;
			std::optional<domain::BusInfo> InfoBusRequest(const Query& query) const;
			std::set<domain::Bus> AllBusesRequest() const;
			void DrawMapRequest(std::ostream& out) const;
			std::optional<TransportRouter::TransportRouteInfo> GetShortestRouteRequest(const Query& query) const;

		private:	
			const TransportCatalogue& catalogue_;
			std::vector<Query> requests_ = {};
			std::unique_ptr<MapRenderer> renderer_;
			std::unique_ptr<TransportRouter> router_;
		};

	}
}

