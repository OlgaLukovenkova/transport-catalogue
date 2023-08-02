#pragma once
#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include <filesystem>

namespace transport_catalogue {

	namespace interfaces {

		class JSONReader {
		public:
			JSONReader(TransportCatalogue& catalogue)
				: catalogue_(catalogue), handler_(catalogue) {
			}

			void LoadData(std::istream& input);
			void PrintAnswers(std::ostream& output);
			std::optional<std::filesystem::path> GetSerializationFile() const;
			RequestHandler& GetHandler();
			const RequestHandler& GetHandler() const;

		private:
			TransportCatalogue& catalogue_; //связываем с готовым каталогом
			RequestHandler handler_; //создается на основе готового каталога
			std::filesystem::path serialization_file_;

			void SetDistancesFromJSON(std::string_view from, const json::Dict& distances);
			void AddDataToCatalogueFromJSON(json::Array& query_queue);
			void AddRequestsToHandlerFromJSON(const json::Array& query_queue);
			void AddSettingsToRendererFromJSON(const json::Dict& json_settings);
			void AddSettingsAndBuildRouterFromJSON(const json::Dict& json_settings);
			void AddSerializationFile(const json::Dict& serialization_settings);
		};
	}
}

