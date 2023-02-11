#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"
// #include "test_framework.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main() {
    // tests::RunInputTests();
    // tests::RunTransportCatalogueTests();
    // tests::RunOutputTests();

    transport::TransportCatalogue tc = readinput::CreateTransportCatalogue(cin);

    size_t n;
    cin >> n;
    cin.get();
    string query;
    string busname;
    string stopname;

    for (size_t i = 0; i < n; ++i) {
        getline(cin, query, ' ');
        if (query == "Bus"sv) {
            getline(cin, busname);
            cout << tc.GetBusInfo(busname) << endl;
        } else {
            getline(cin, stopname);
            cout << tc.GetStopInfo(stopname) << endl;
        }
    }
}