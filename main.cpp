#include <fstream>
#include <iostream>
#include <string_view>

#include "transport_catalogue.h"
#include "json_reader.h"
#include "serialization.h"

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

void RunMakeBase() {
    transport_catalogue::TransportCatalogue catalogue;
    transport_catalogue::interfaces::JSONReader reader(catalogue);
    reader.LoadData(std::cin); 
    auto path = reader.GetSerializationFile();
    if (!path) {
        std::cerr << "No serialization settings\n"sv;
        return;
    }
    transport_catalogue::interfaces::Serializator serializator(catalogue, reader.GetHandler(), *path); 
    if (!serializator.Serialize()) {
        std::cerr << "Serialization failed\n"sv;
        return;
    }
}

void RunProcessRequests() {
    transport_catalogue::TransportCatalogue catalogue;
    transport_catalogue::interfaces::JSONReader reader(catalogue);
    reader.LoadData(std::cin);
    auto path = reader.GetSerializationFile();
    transport_catalogue::interfaces::Deserializator deserializator(catalogue, reader.GetHandler(), *path);
    deserializator.Deserialize();
    reader.PrintAnswers(std::cout);
}

int main(int argc, char* argv[]) {
    /*if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        RunMakeBase();
    }
    else if (mode == "process_requests"sv) {
        RunProcessRequests();
    }
    else {
        PrintUsage();
        return 1;
    }*/
    RunMakeBase();
    RunProcessRequests();
}

