#include "request_handler.h"
#include "json_reader.h"
// #include "test_framework.h"
#include "json_builder.h"

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

    request_handler.BuildTransportDatabase();
    request_handler.BuildMapRenderer();
    request_handler.BuildRouter();

    // формируется ответный json
    request_handler.SolveStatRequests();

    // печатаем json-ответ в stdout    
    request_handler.PrintSolution(cout);

    // в поток выводится без экранирующих '\'
    // в файл если выводить, то с экранирующими '\'
}

// {
//     JsonReader json_reader;

//     // считываем json из файла
//     int n; cin >> n;
//     ifstream input("./tst/ijson"s + to_string(n) + ".json"s);
//     json_reader.LoadJSON(input);
    
//     RequestHandler request_handler(json_reader);

//     // хочется это распихать в ветки метода SolveStatRequests: если запрос на Map есть, то и MapRenderer создается, а потом уже
//     // юзается. нет запроса -- нет и построения MapRenderer -- и так со всеми остальными
//     request_handler.BuildTransportDatabase();
//     request_handler.BuildMapRenderer();
//     request_handler.BuildRouter();

//     request_handler.SolveStatRequests();

//     // печатаем json-ответ в файл
//     ofstream output1("./tst/ojson"s + to_string(n) + ".json"s);
//     request_handler.PrintSolution(output1);
//     request_handler.PrintSolution(cout);
//     cout << endl << endl << endl;
//     ofstream output2("./tst/my_tests_before_s13/osvg"s + to_string(n) + ".svg"s);
//     // печатаем карту в файл
//     request_handler.RenderMap(output2);
//     request_handler.RenderMap(cout);
// }
}