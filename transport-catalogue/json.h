#pragma once

#include <istream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

class Node;

namespace detail {

Node LoadNode(std::istream& input);
Node LoadNull(std::istream& input);
Node LoadNumber(std::istream& input);
Node LoadString(std::istream& input);
Node LoadBool(std::istream& input);
Node LoadArray(std::istream& input);
Node LoadDict(std::istream& input);

}

using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

bool operator==(const Dict& lhs, const Dict& rhs);
bool operator==(const Array& lhs, const Array& rhs);
bool operator!=(const Dict& lhs, const Dict& rhs);
bool operator!=(const Array& lhs, const Array& rhs);

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
    using Value = std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>;

    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;

    Node();
    Node(std::nullptr_t);
    Node(int value);
    Node(double value);
    Node(std::string value);
    Node(bool value);
    Node(Array array);
    Node(Dict map);

    // Следующие методы Node сообщают, хранится ли внутри значение некоторого типа:
    bool IsNull() const;
    bool IsInt() const;
    bool IsDouble() const; // Возвращает true, если в Node хранится int либо double.
    bool IsPureDouble() const; // Возвращает true, если в Node хранится double.
    bool IsString() const;
    bool IsBool() const;
    bool IsArray() const;
    bool IsMap() const;

    // Ниже перечислены методы, которые возвращают хранящееся внутри Node значение заданного типа. Если внутри содержится значение другого типа, должно выбрасываться исключение std::logic_error.
    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const; // Возвращает значение типа double, если внутри хранится double либо int. В последнем случае возвращается приведённое в double значение.
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    const Value& GetValue() const { return value_; }

private:
    Value value_;
};

class Document {
public:

    bool operator==(const Document& other);
    bool operator!=(const Document& other);

    explicit Document(Node root);

    const Node& GetRoot() const;

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}