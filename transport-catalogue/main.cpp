#include "request_handler.h"
#include "json_reader.h"
// #include "test_framework.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main() {
// {    
//     tests::RunInputTests();
//     tests::RunTransportCatalogueTests();
//     tests::RunOutputTests();
// }

{
    JsonReader json_reader;
    // считываем json из stdin
    json_reader.LoadJSON(cin);
    
    // создаем экземпляр фасада, который упростит взаимодействие классов при помощи агрегации
    RequestHandler request_handler(json_reader);

    // метод разберется, какие методы для каких запросов обработчика json надо применить
    request_handler.BuildTransportDatabase();
    request_handler.BuildMapRenderer();

    // формируется ответный json
    request_handler.SolveStatRequests();

    // печатаем json-ответ в stdout    
    request_handler.PrintSolution(cout);
}


// {
//     JsonReader json_reader;

//     // считываем json из файла
//     int n; cin >> n;
//     ifstream input("./tst/my_tests/ijson"s + to_string(n) + ".txt"s);
//     json_reader.LoadJSON(input);
    
//     RequestHandler request_handler(json_reader);

//     request_handler.BuildTransportDatabase();
//     request_handler.BuildMapRenderer();

//     request_handler.SolveStatRequests();

//     // печатаем json-ответ в файл   
//     ofstream output1("./tst/my_tests/ojson"s + to_string(n) + ".json"s);
//     request_handler.PrintSolution(output1);

//     ofstream output2("./tst/my_tests/osvg"s + to_string(n) + ".svg"s);
//     // печатаем карту в файл
//     request_handler.RenderMap(output2);
// }

}