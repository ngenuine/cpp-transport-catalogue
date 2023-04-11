#include "json_builder.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <optional>

using namespace std::literals;

namespace json {

namespace details {

std::string Visitor::operator()(bool b) const {
    if (b) {
        return "true"s;
    }

    return "false"s;
}

std::string Visitor::operator()(std::nullptr_t) const {
    return "null"s;
}

std::string Visitor::operator()(int num) const {
    return std::to_string(num);
}

std::string Visitor::operator()(double num) const {
    return std::to_string(num);
}

std::string Visitor::operator()(std::string s) const {
    return s;
}

std::string Visitor::operator()(Array /* arr */) const {
    return "Array"s;
}

std::string Visitor::operator()(Dict /* dict */) const {
    return "Dict"s;
}

// ========================= BaseItemContext =========================
BaseItemContext::BaseItemContext(json::Builder& builder)
    : builder_(builder) {}
DictItemContext BaseItemContext::StartDict() {
    return DictItemContext(builder_.StartDict());
}

ArrayItemContext BaseItemContext::StartArray() {
    return ArrayItemContext(builder_.StartArray());
}

json::Builder& BaseItemContext::EndDict() {
    return builder_.EndDict();
}

json::Builder& BaseItemContext::EndArray() {
    return builder_.EndArray();
}

KeyItemContext BaseItemContext::Key(std::string key) {
    return KeyItemContext(builder_.Key(key));
}

json::Builder& BaseItemContext::Get() {
    return builder_;
}

// ========================= KeyItemContext =========================

KeyItemContext::KeyItemContext(json::Builder& builder)
    : BaseItemContext(builder) {}

ValueItemContextAfterKey KeyItemContext::Value(Node value) {
    Get().Value(value);
    return ValueItemContextAfterKey(Get());
}

// DictItemContext KeyItemContext::StartDict() {
//     return DictItemContext(builder_.StartDict());
// }

// ArrayItemContext KeyItemContext::StartArray() {
//     return ArrayItemContext(builder_.StartArray());
// }

// ========================= ValueItemContextAfterValue =========================
ValueItemContextAfterValue::ValueItemContextAfterValue(json::Builder& builder)
    : BaseItemContext(builder) {}

ValueItemContextAfterValue ValueItemContextAfterValue::Value(json::Node value) {
    Get().Value(value);
    return ValueItemContextAfterValue(Get());
}

// DictItemContext ValueItemContextAfterValue::StartDict() {
//     return DictItemContext(builder_.StartDict());
// }

// ArrayItemContext ValueItemContextAfterValue::StartArray() {
//     return ArrayItemContext(builder_.StartArray());
// }

// json::Builder& ValueItemContextAfterValue::EndArray() {
//     return builder_.EndArray();
// }

// ========================= ValueItemContext =========================
ValueItemContext::ValueItemContext(json::Builder& builder)
    : BaseItemContext(builder) {}

ValueItemContextAfterValue ValueItemContext::Value(json::Node value) {
    Get().Value(value);
    return ValueItemContextAfterValue(Get());
}

// DictItemContext ValueItemContext::StartDict() {
//     return DictItemContext(builder_.StartDict());
// }

// ArrayItemContext ValueItemContext::StartArray() {
//     return ArrayItemContext(builder_.StartArray());
// }

// json::Builder& ValueItemContext::EndArray() {
//     return builder_.EndArray();
// }

json::Node ValueItemContext::Build() {
    return Get().Build();
}

// ========================= ValueItemContextAfterKey =========================
ValueItemContextAfterKey::ValueItemContextAfterKey(json::Builder& builder)
    : BaseItemContext(builder) {}

// KeyItemContext ValueItemContextAfterKey::Key(std::string key) {
//     return KeyItemContext(builder_.Key(key));
// }

// json::Builder& ValueItemContextAfterKey::EndDict() {
//     return builder_.EndDict();
// }

// ========================= ArrayItemContext =========================

ArrayItemContext::ArrayItemContext(Builder& builder)
    : BaseItemContext(builder) {}

ValueItemContext ArrayItemContext::Value(json::Node value) {
    return ValueItemContext(Get().Value(value));
}

// DictItemContext ArrayItemContext::StartDict() {
//     return DictItemContext(builder_.StartDict());
// }

// ArrayItemContext ArrayItemContext::StartArray() {
//     return ArrayItemContext(builder_.StartArray());
// }

// Builder& ArrayItemContext::EndArray() {
//     return builder_.EndArray();
// }

// ========================= DictItemContext =========================

DictItemContext::DictItemContext(json::Builder& builder)
    : BaseItemContext(builder) {}

// KeyItemContext DictItemContext::Key(std::string key) {
//     return KeyItemContext(builder_.Key(key));
// }

// Builder& DictItemContext::EndDict() {
//     return builder_.EndDict();
// }

} // namespace details

details::KeyItemContext Builder::Key(std::string key) {
    // когда мы имеем право положить std::string key в json: когда последний элемент в стеке словарь
    // (все пары ключ-значение до этого ключа уже находятся в словаре, а это новый ключ следующей такой пары)
    if (!nodes_stack_.empty() && !nodes_stack_.back()->IsDict()) {
        throw std::logic_error("Dict was not opened before this key: "s + key);
    }

    nodes_stack_.emplace_back(new Node(key));

    return details::KeyItemContext(*this);
}

details::ValueItemContext Builder::Value(Node value) {
    // когда мы имеем право положить value в json: когда стек пуст, там массив или строка (key из пары key-value)
    if (!is_root_set_) {
        root_ = value;
        is_root_set_ = true;
    } else if (!nodes_stack_.empty() && nodes_stack_.back()->IsString()) {
        // если последний элемент строка, значит в до нее возможно был открыт словарь
        std::string key = nodes_stack_.back()->AsString();
        delete nodes_stack_.back();
        nodes_stack_.pop_back();

        // Проверяем, что последний элемент действительно словарь
        if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict()) {
            // добавляем в словарь значением
            std::get<Dict>(nodes_stack_.back()->Get())[key] = std::move(value);
        } else {
            throw std::logic_error("Dict was not opened before this key: "s + key);
        }
    } else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
        // последний элемент в стеке это Array; тогда положим value в Array
        std::get<Array>(nodes_stack_.back()->Get()).emplace_back(std::move(value));
    } else {
        // попытка положить какое-то значение в стэк, если последний элемент это
        // не строка или массив, это ошибка, формируем понятное сообщение об ошибке
        std::string value_as_string = std::visit(details::Visitor{}, value.GetValue());
        throw std::logic_error(value_as_string + " as Value cannot be a member of json at this position"s);
    }

    return details::ValueItemContext(*this);
}

details::DictItemContext Builder::StartDict() {
    // когда мы имеем право начать контейнер словарь: когда в стэке пусто, последний элемент Array или std::string (ключ словаря)
    if (!is_root_set_) {
        root_ = Dict{};
        nodes_stack_.push_back(&root_);

        is_root_set_ = true;
    } else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
        Array& arr = std::get<Array>(nodes_stack_.back()->Get());
        arr.emplace_back(Dict{});

        nodes_stack_.push_back(&arr.back());
    } else if (!nodes_stack_.empty() && nodes_stack_.back()->IsString()) {
        std::string key = nodes_stack_.back()->AsString();
        delete nodes_stack_.back();
        nodes_stack_.pop_back();

        if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict()) {
            Dict& dict = std::get<Dict>(nodes_stack_.back()->Get());
            dict[key] = Dict{};

            nodes_stack_.push_back(&dict[key]);
        } else {
            throw std::logic_error("Dict was not opened before this key: "s + key);
        }
    } else {
        throw std::logic_error("Cannot StartDict"s);
    }

    return details::DictItemContext(*this);
}

Builder& Builder::EndDict() {
    // когда мы имеем право закончить контейнер словарь: когда последний элемент в стэке словарь
    if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict()) {
        nodes_stack_.pop_back();
    } else {
        throw std::logic_error("Dict was not opened or Array was not closed"s);
    }
    
    return *this;
}

details::ArrayItemContext Builder::StartArray() {
    // когда мы имеем право начать контейнер словарь: когда в стэке пусто, последний элемент Array или std::string (ключ словаря)
    if (!is_root_set_) {
        root_ = Array{};
        nodes_stack_.push_back(&root_);
    
        is_root_set_ = true;
    } else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
        Array& arr = std::get<Array>(nodes_stack_.back()->Get());
        arr.emplace_back(Array{});

        nodes_stack_.push_back(&arr.back());
    } else if (!nodes_stack_.empty() && nodes_stack_.back()->IsString()) {
        std::string key = nodes_stack_.back()->AsString();
        delete nodes_stack_.back();
        nodes_stack_.pop_back();

        if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict()) {
            Dict& dict = std::get<Dict>(nodes_stack_.back()->Get());
            dict[key] = Array{};

            nodes_stack_.push_back(&dict[key]);
        } else {
            throw std::logic_error("Array was not opened before this key: "s + key);
        }
    } else {
        throw std::logic_error("Cannot start Array"s);
    }

    return details::ArrayItemContext(*this);
}

Builder& Builder::EndArray() {
    // когда мы имеем право закончить контейнер массив: когда последний элемент в стэке массив
    if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
        nodes_stack_.pop_back();
    } else {
        throw std::logic_error("Array was not opened or Dict was not closed"s);
    }

    return *this;
}

Node Builder::Build() {
    if (!is_root_set_ || nodes_stack_.size() != 0) {
        throw std::logic_error("Nothing to build or not all containers are closed"s);
    }

    return root_;
}

} // namespace json