#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

int main() {
    char letter_case;
    cin >> letter_case;

    int number_case;
    cin >> number_case;
    string ncase = to_string(number_case);

    string input_file_name = "tst/ts"s;
    input_file_name += letter_case;
    input_file_name += "_case";
    input_file_name += ncase;
    input_file_name.insert(13, "_input.txt"s, 0, 10);

    fstream input(input_file_name);
    transport::TransportCatalogue tc = readinput::CreateTransportCatalogue(input);

    size_t n;
    input >> n;
    input.get();
    string query;
    string busname;
    string stopname;

    ofstream output("tst/output.txt"s);

    for (size_t i = 0; i < n; ++i) {
        getline(input, query, ' ');
        if (query == "Bus"sv) {
            getline(input, busname);
            output << tc.GetBusInfo(busname) << endl;
        } else {
            getline(input, stopname);
            output << tc.GetStopInfo(stopname) << endl;
        }
    }

    input.close();
    output.close();

    string output_file_name = "tst/ts"s;
    output_file_name += letter_case;
    output_file_name += "_case";
    output_file_name += ncase;
    output_file_name.insert(13, "_output.txt"s, 0, 11);

    fstream my_ans("tst/output.txt"s);
    fstream etalon(output_file_name);
    string my_line;
    string true_line;

    int counter = 0;
    int mistakes = 0;
    while(getline(my_ans, my_line) && getline(etalon, true_line)) {
        if (my_line != true_line) {
            cout << "false in "sv << counter << " line"sv << endl;
        }
        
        ++counter;
    }

    if (mistakes == 0) {
        cout << "OK"sv << endl;
    }

    my_ans.close();
    etalon.close();
}