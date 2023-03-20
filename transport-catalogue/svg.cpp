#include "svg.h"
#include <cmath>

namespace svg {

using namespace std::literals;

std::ostream& operator<<(std::ostream& out, const StrokeLineCap& stroke_line_cap) {
    switch (stroke_line_cap) {
        case StrokeLineCap::BUTT:
            out << "butt";
            break;
        case StrokeLineCap::ROUND:
            out << "round";
            break;
        case StrokeLineCap::SQUARE:
            out << "square";
            break;
        default:
            break;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& stroke_line_join) {
    switch (stroke_line_join) {
        case StrokeLineJoin::ARCS:
            out << "arcs";
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel";
            break;
        case StrokeLineJoin::MITER:
            out << "miter";
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip";
            break;
        case StrokeLineJoin::ROUND:
            out << "round";
            break;
        default:
            break;
    }

    return out;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\" "sv;
    
    // Выводим атрибуты, унаследованные от PathProps
    this->RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Polyline ------------------
Polyline& Polyline::AddPoint(Point point)  {

    points_.push_back(std::move(point));

    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    
    for (auto it = points_.begin(); it != --points_.end(); ++it) {
        out << it->x << ',' << it->y << ' ';
    }

    out << points_.back().x << ',' << points_.back().y;

    out << "\" "sv;

    // Выводим атрибуты, унаследованные от PathProps
    this->RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(std::string data) {
    // Символы ", <, >, ' и & имеют особое значение и при выводе должны экранироваться
    std::string prepared_text = ""s;
    for (const char c : data) {
        if (remarkable_symbols.count(c) == 1) {
            prepared_text += remarkable_symbols.at(c);
        } else {
            prepared_text += c;
        }
    }

    data_ = prepared_text;
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text "sv;
    
    // Выводим атрибуты, унаследованные от PathProps
    this->RenderAttrs(out);

    out << "x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" "sv;
    out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
    out << "font-size=\""sv << font_size_ << "\""sv;
    if (font_family_.size() != 0) {
        out << " font-family=\""sv << font_family_ << "\""sv;
    }

    if (font_weight_.size() != 0) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }


    out << '>' << data_;
    out << "</text>"sv;
}

// ---------- Document ------------------
void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    
    RenderContext ctx(out);

    for (const auto& object : objects_) {
        // вывод содержимого документа
        (*object).Render(ctx);
    }
    out << "</svg>"sv;
}

}  // namespace svg