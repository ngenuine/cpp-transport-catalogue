// напишите решение с нуля
// код сохраните в свой git-репозиторий

#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

int main() {
    
    TransportCatalogue tc = CreateTransportCatalogue(cin);

    size_t n;
    cin >> n;
    string _;
    string busname;

    for (size_t i = 0; i < n; ++i) {
        getline(cin, _, ' ');
        getline(cin, busname);
        cout << tc.GetBusInfo(busname) << endl;
    }
}