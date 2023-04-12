#include "json.h"
#include <vector>
#include <string>

using namespace std::literals;

namespace json {

class Builder;

namespace details {

struct Visitor {
    std::string operator()(bool b) const;
    std::string operator()(std::nullptr_t) const;
    std::string operator()(int num) const;
    std::string operator()(double num) const;
    std::string operator()(std::string s) const;
    std::string operator()(Array /* arr */) const;
    std::string operator()(Dict /* dict */) const;
};

class KeyItemContext;
class ValueItemContextAfterValue;
class ValueItemContext;
class ArrayItemContext;
class DictItemContext;

// ========================= BaseItemContext =========================
class BaseItemContext {
public:
    explicit BaseItemContext(json::Builder& builder);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    json::Builder& EndDict();
    json::Builder& EndArray();
    KeyItemContext Key(std::string key);

    json::Builder& Get();
private:
    json::Builder& builder_;
};

// ========================= KeyItemContext =========================

class KeyItemContext : public BaseItemContext {
public:

    explicit KeyItemContext(json::Builder& builder);

    // некоторые члены класс реализует самостоятельно
    DictItemContext Value(Node value);

    // закомментированные члены класса реализованы в базовом классе
    // DictItemContext StartDict();
    // ArrayItemContext StartArray();

    // остальные члены базового класса не предполагают их использование в наследнике
    json::Builder& EndDict() = delete;
    json::Builder& EndArray() = delete;
    KeyItemContext Key(std::string key) = delete;

// private:
//     json::Builder& builder_;
};

// ========================= ValueItemContextAfterValue =======================
class ValueItemContextAfterValue : public BaseItemContext {
public:

    explicit ValueItemContextAfterValue(json::Builder& builder);

    // некоторые члены класс реализует самостоятельно
    ValueItemContextAfterValue Value(json::Node value);

    // закомментированные члены класса реализованы в базовом классе
    // DictItemContext StartDict();
    // ArrayItemContext StartArray();
    // json::Builder& EndArray();

    // остальные члены базового класса не предполагают их использование в наследнике
    json::Builder& EndDict() = delete;
    KeyItemContext Key(std::string key) = delete;

// private:
//     json::Builder& builder_;
};

// ========================= ValueItemContext =========================
class ValueItemContext : public BaseItemContext {
public:

    explicit ValueItemContext(json::Builder& builder);

    // некоторые члены класс реализует самостоятельно
    ValueItemContextAfterValue Value(json::Node value);

    // закомментированные члены класса реализованы в базовом классе
    // DictItemContext StartDict();
    // ArrayItemContext StartArray();
    // json::Builder& EndArray();
    json::Node Build();

    // остальные члены базового класса не предполагают их использование в наследнике
    json::Builder& EndDict() = delete;
    KeyItemContext Key(std::string key) = delete;

// private:
//     json::Builder& builder_;
};

// ========================= ArrayItemContext =========================

class ArrayItemContext : public BaseItemContext {
public:

    explicit ArrayItemContext(json::Builder& builder);

    // некоторые члены класс реализует самостоятельно
    ValueItemContextAfterValue Value(Node value);

    // закомментированные члены класса реализованы в базовом классе
    // DictItemContext StartDict();
    // ArrayItemContext StartArray();
    // json::Builder& EndArray();

    // остальные члены базового класса не предполагают их использование в наследнике
    json::Builder& EndDict() = delete;
    KeyItemContext Key(std::string key) = delete;

// private:
//     json::Builder& builder_;
};

// ========================= DictItemContext =========================

class DictItemContext : public BaseItemContext {
public:

    explicit DictItemContext(json::Builder& builder);

    // закомментированные члены класса реализованы в базовом классе
    // KeyItemContext Key(std::string key);
    // json::Builder& EndDict();

    // остальные члены базового класса не предполагают их использование в наследнике
    DictItemContext StartDict() = delete;
    ArrayItemContext StartArray() = delete;
    json::Builder& EndArray() = delete;

// private:
//     json::Builder& builder_;
};

} // namespace details

class Builder {
public:

    details::KeyItemContext Key(std::string key);
    details::ValueItemContext Value(Node value);
    details::DictItemContext StartDict();
    Builder& EndDict();
    details::ArrayItemContext StartArray();
    Builder& EndArray();
    Node Build();

private:
    Node root_ = nullptr;
    std::vector<Node*> nodes_stack_;

    // без этого поля нельзя понять, что корень был инициализирован, ведь проверку root_.IsNull() нельзя использовать,
    // так как nullptr это валидное значение корня; и если я пишу в Build() "if (root_.IsNull() || nodes_stack_.size() != 0)"
    // то падаю в исключение, хотя но root_ запросто может быть IsNull(), нарпимер так: Builder{}.Value(nullptr).Build()
    bool is_root_set_ = false;
};

} // namespace json