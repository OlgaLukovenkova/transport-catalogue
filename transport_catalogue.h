#pragma once

#include <string>
#include <deque>
#include <unordered_map>
#include <optional>
#include <vector>
#include <set>

#include "domain.h"

namespace transport_catalogue {
	using domain::Stop;
	using domain::Bus;
	using domain::BusInfo;

	using StopPair = std::pair<const Stop*, const Stop*>;

	class TransportCatalogue {
	private:
		struct Hasher {
			size_t operator() (const StopPair& obj) const {
				std::hash<const void*> hasher;
				return hasher(obj.first) + hasher(obj.second) * 37;
			}
		};

	public:		
		void AddStop(const Stop& stop);
		void AddBus(const Bus& bus);
		std::optional<const Stop*> FindStopByName(std::string_view stop) const;
		std::optional<const Bus*> FindBusByName(std::string_view bus) const;
		std::optional<BusInfo> GetInfoAboutBus(std::string_view bus) const;
		std::optional<std::set<std::string_view>> GetBusesByStop(std::string_view stop) const;
		void SetDistance(std::string_view from, std::string_view to, size_t distance);
		size_t GetDistance(std::string_view from, std::string_view to) const;
		const std::deque<Bus>& GetAllBuses() const;
		const std::deque<Stop>& GetAllStops() const;
		const std::unordered_map<StopPair, size_t, Hasher>& GetAllDistances() const {
			return distances_;
		}

	private:		
		std::deque<Stop> stops_;
		std::unordered_map<std::string_view, const Stop*> stops_by_name_;
		std::deque<Bus> buses_;
		std::unordered_map<std::string_view, const Bus*> buses_by_name_;
		std::unordered_map<const Stop*, std::set<std::string_view>> buses_by_stop_;
		std::unordered_map<StopPair, size_t, Hasher> distances_;
	};

	namespace tests {

		void AddStop();
		void AddBus();
		void FindStop();
		void FindBus();
		void GetInfoAboutBus();
		void GetBusesByStop();
		void AddGetDistance();

	}
}
