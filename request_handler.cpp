#include "request_handler.h"
#include <algorithm>

namespace transport_catalogue {

	namespace interfaces {

		void RequestHandler::Reserve(size_t capacity) {
			requests_.reserve(capacity);
		}

		void RequestHandler::AddRequest(const RequestHandler::Query& request) {
			requests_.push_back(request);
		}

		const MapRenderer::Settings& RequestHandler::GetRendererSettings() const {
			if (!renderer_) {
				throw std::logic_error("The renderer was not created");
			}
			return renderer_->GetSettings();
		}

		const std::vector<RequestHandler::Query>& RequestHandler::GetRequests() const {
			return requests_;
		}

		std::optional<std::set<std::string_view>> RequestHandler::InfoStopRequest(const RequestHandler::Query& query) const {
			return catalogue_.GetBusesByStop(query.parameters.at(0));
		}

		std::optional<domain::BusInfo> RequestHandler::InfoBusRequest(const RequestHandler::Query& query) const {
			return catalogue_.GetInfoAboutBus(query.parameters.at(0));
		}

		std::set<domain::Bus> RequestHandler::AllBusesRequest() const {
			return { catalogue_.GetAllBuses().begin(), catalogue_.GetAllBuses().end() };
		}

		void RequestHandler::DrawMapRequest(std::ostream& out) const {
			if (!renderer_) {
				throw std::logic_error("The renderer was not created");
			}
			renderer_->DrawMap(out, AllBusesRequest());
		}

		std::optional<TransportRouter::TransportRouteInfo> RequestHandler::GetShortestRouteRequest(const RequestHandler::Query& query) const {
			if (!router_) {
				throw std::logic_error("The router was not created");
			}
			return router_->GetShortestRoute(query.parameters.at(0), query.parameters.at(1));
		}

	}
}
