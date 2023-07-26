#include "request_handler.h"
#include "json_reader.h"
#include "json_builder.h"
#include "serialization.h"
// #include "test_framework.h"

#include <iostream>
#include <fstream>
#include <string>
#include <string_view>

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
// {    
//     tests::RunInputTests();
//     tests::RunTransportCatalogueTests();
//     tests::RunOutputTests();
// }

    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        JsonReader json_reader;
        json_reader.LoadJSON(std::cin, ReadMode::MAKE_BASE);

        TransportData transport_data;
        RequestHandler request_handler(json_reader, transport_data, ReadMode::MAKE_BASE);

        serialization::Serializator serializator(transport_data);

        std::ofstream binary_output(json_reader.GetOutputFilepath(), std::ios::binary);
        serializator.Serialize(binary_output);

    } else if (mode == "process_requests"sv) {
        JsonReader json_reader;
        json_reader.LoadJSON(std::cin, ReadMode::PROCESS_REQUEST);

        TransportData transport_data;
        serialization::Serializator serializator(transport_data);

        std::ifstream binary_input(json_reader.GetInputFilepath(), std::ios::binary);
        serializator.Deserialize(binary_input);

        RequestHandler request_handler(json_reader, transport_data, ReadMode::PROCESS_REQUEST);
        request_handler.SolveStatRequests();
        request_handler.PrintSolution(std::cout);
        // request_handler.RenderMap(std::cout);
    } else {
        PrintUsage();
        return 1;
    }
}