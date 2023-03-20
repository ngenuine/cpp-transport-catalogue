#include "json.h"
#include <unordered_set>

using namespace std;

namespace json {

namespace detail {

Node LoadNull(istream& input) {
    std::string emptyvoid = ""s;

    const size_t null_length = 4;
    
    char c;

    for (; emptyvoid.length() < null_length && input >> c;) {
        emptyvoid.push_back(c);
    }
    
    if (emptyvoid == "null"s) {
        return Node(nullptr);
    }

    throw ParsingError("Failed to read null from stream"s);
}

Node LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return Node(std::stoi(parsed_num));
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }

        return Node(std::stod(parsed_num));
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
Node LoadString(std::istream& input) {
    using namespace std::literals;
    
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return Node(move(s));
}

Node LoadBool(istream& input) {
    std::string some_bool = ""s;

    char c;
    input >> c;

    size_t bool_length = (c == 't') ? 4 : 5;
    some_bool.push_back(c);

    for (; some_bool.length() < bool_length && input >> c;) {
        some_bool.push_back(c);
    }
    
    if (some_bool == "true"s) {
        return Node(true);
    } else if (some_bool == "false"s) {
        return Node(false);
    }


    throw ParsingError("Failed to read bool from stream"s);
}

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;

    char c;

    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    if (c != ']') {
        throw ParsingError("Failed to read array from stream"s);
    }

    return Node(move(result));
}

Node LoadDict(istream& input) {
    Dict result;

    char c;

    for (; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;

        if (c != ':') {
            throw ParsingError("Failed to read array from stream"s);
        }

        result.insert({move(key), LoadNode(input)});
    }

    if (c != '}') {
        throw ParsingError("Failed to read array from stream"s);
    }

    return Node(move(result));
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    } else {
        input.putback(c);
        return LoadNumber(input);
    }
}

} // namespace detail

bool Node::operator==(const Node& other) const {

    if (this->IsNull() && other.IsNull()) {
        return true;
    } else if (this->IsInt() && other.IsInt()) {
        return this->AsInt() == other.AsInt();
    } else if (this->IsPureDouble() && other.IsPureDouble()) {
        return std::abs(this->AsDouble() - other.AsDouble()) < 1e-6;
    } else if (this->IsString() && other.IsString()) {
        return this->AsString() == other.AsString();
    } else if (this->IsBool() && other.IsBool()) {
        return this->AsBool() == other.AsBool();
    } else if (this->IsArray() && other.IsArray()) {
        return this->AsArray() == other.AsArray();
    } else if (this->IsMap() && other.IsMap()) {
        return this->AsMap() == other.AsMap();
    }

    return false;
}

bool Node::operator!=(const Node& other) const {
    return !(operator==(other));
}

bool Document::operator==(const Document& other) {
    return this->GetRoot() == other.GetRoot();
}
bool Document::operator!=(const Document& other) {
    return !(operator==(other));
}

bool operator==(const Dict& lhs, const Dict& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (const auto& [key, lhs_node] : lhs) {
        if (rhs.count(key) != 1) {
            return false;
        }

        const Node& rhs_node = rhs.at(key);

        if (lhs_node != rhs_node) {
            return false;
        }
    }

    return true;
}

bool operator!=(const Dict& lhs, const Dict& rhs) {
    return !(operator==(lhs, rhs));
}

bool operator==(const Array& lhs, const Array& rhs) {

    auto find_positions_same_node = [&rhs](const Node& node) {

        unordered_set<size_t> positions;

        for (size_t pos = 0; pos < rhs.size(); ++pos) {
            if (node == rhs[pos]) {
                positions.insert(pos);
            }
        }

        return positions;
    };

    if (lhs.size() != rhs.size()) {
        return false;
    }

    unordered_set<size_t> already_used;

    for (const Node& node : lhs) {
        
        // все позиции равых node элементов
        unordered_set<size_t> positions = find_positions_same_node(node);

        bool same_node_founded = false;

        for (const auto pos : positions) {
            if (already_used.count(pos) == 0) {
                // первый равный node элемент нас устраивает
                already_used.insert(pos); 
                same_node_founded = true;

                // просматривать другие same_node не надо
                break;
            }
        }

        if (!same_node_founded) {
            // для текущего node в одном Array не нашлось равного node
            // в другом Array, значит Array не равны
            return false;
        }
    }

    return true;
}

bool operator!=(const Array& lhs, const Array& rhs) {
    return !(operator==(lhs, rhs));
}

Node::Node()
    : value_(nullptr) {
}

Node::Node(std::nullptr_t)
    : value_(nullptr) {
}

Node::Node(int value)
    : value_(value) {
}

Node::Node(double value)
    : value_(value) {
}

Node::Node(string value)
    : value_(move(value)) {
}

Node::Node(bool value)
    : value_(value) {
}

Node::Node(Array array)
    : value_(move(array)) {
}

Node::Node(Dict map)
    : value_(move(map)) {
}

// Следующие методы Node сообщают, хранится ли внутри значение некоторого типа
bool Node::IsNull() const {
    return holds_alternative<nullptr_t>(value_);
}

bool Node::IsInt() const {
    return holds_alternative<int>(value_);
}

// Возвращает true, если в Node хранится int либо double.
bool Node::IsDouble() const {
    bool is_int = holds_alternative<int>(value_);
    bool is_double = holds_alternative<double>(value_);

    return is_int || is_double;
}

// Возвращает true, если в Node хранится double.
bool Node::IsPureDouble() const {
    return holds_alternative<double>(value_);
}

bool Node::IsString() const {
    return holds_alternative<string>(value_);
}

bool Node::IsBool() const {
    return holds_alternative<bool>(value_);
}

bool Node::IsArray() const {
    return holds_alternative<Array>(value_);
}

bool Node::IsMap() const {
    return holds_alternative<Dict>(value_);
}

// Ниже перечислены методы, которые возвращают хранящееся внутри Node значение заданного типа. Если внутри содержится значение другого типа, должно выбрасываться исключение std::logic_error

int Node::AsInt() const {
    if (std::holds_alternative<int>(value_)) {
        return get<int>(value_);
    }
    
    throw std::logic_error("This node not contain int");
}

double Node::AsDouble() const {
    const auto* value = get_if<int>(&value_);
    if (value) {
        return static_cast<double>(get<int>(value_));
    } else if (holds_alternative<double>(value_)) {
        return get<double>(value_);
    }

    throw std::logic_error("This node not contain double");
}

bool Node::AsBool() const {
    if (holds_alternative<bool>(value_)) {
        return get<bool>(value_);
    }
    
    throw std::logic_error("This node not contain bool");
}

const string& Node::AsString() const {
    if (std::holds_alternative<string>(value_)) {
        return std::get<string>(value_);
    }
    
    throw std::logic_error("This node not contain string");
}

const Array& Node::AsArray() const {
    if (holds_alternative<Array>(value_)) {
        return get<Array>(value_);
    }
    
    throw std::logic_error("This node not contain array");
}

const Dict& Node::AsMap() const {
    if (holds_alternative<Dict>(value_)) {
        return get<Dict>(value_);
    }
    
    throw std::logic_error("This node not contain map");
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{detail::LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {

    const Node& node = doc.GetRoot();

    if (node.IsNull()) {
        output << "null"s;
    } else if (node.IsInt()) {
        output << node.AsInt();
    } else if (node.IsPureDouble()) {
        output << node.AsDouble();
    } else if (node.IsString()) {

        output << '\"';

        for (char c : node.AsString()) {
            if (c == '\t') {
                output << '\t';
            } else if (c == '\\') {
                output << '\\' << '\\';
            } else if (c == '\"') {
                output << '\\' << c;
            } else if (c == '\r') {
                output << '\\' << 'r';
            } else if (c == '\n') {
                output << '\\' << 'n';
            } else {
                output << c;
            }
        }

        output << '\"';

    } else if (node.IsBool()) {
        output << boolalpha << node.AsBool();
    } else if (node.IsArray()) {

        output << '[';
        bool is_first = true;
        for (const Node& arr_node : node.AsArray()) {
            if (!is_first) {
                output << ", "sv;
            }

            Print(Document{arr_node}, output);
            is_first = false;
        }

        output << ']';

    } else if (node.IsMap()) {
        output << '{';
        bool is_first = true;
        for (const auto& [key, value] : node.AsMap()) {
            if (!is_first) {
                output << ", "sv;
            }

            Print(Document{key}, output);
            output << ": "sv;
            Print(Document{value}, output);
            is_first = false;
        }
        output << '}';
    }
}

} // namespace json