#include "domain.h"

Request::Request() = default;

Request::~Request() = default;

Request::Request(int id_, std::string type_)
    : id(id_)
    , type(std::move(type_)) {}

Transport::Transport(int id_, std::string type_, std::string name_)
    : Request(id_, std::move(type_))
    , name(std::move(name_)) {}

Transport::Transport(Transport&& other) {
    name = std::move(other.name);
    id = other.id;
    type = std::move(other.type);
}

Map::Map(int id_, std::string type_)
    : Request(id_, std::move(type_)) {}

Map::Map(Map&& other) {
    id = other.id;
    type = std::move(other.type);
}

Route::Route(int id_, std::string type_, std::string from_, std::string to_)
    : Request(id_, std::move(type_))
    , from(std::move(from_))
    , to(std::move(to_)) {}

Route::Route(Route&& other) {
    id = other.id;
    type = std::move(other.type);
    from = std::move(other.from);
    to = std::move(other.to);
}