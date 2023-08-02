#include "svg.h"
#include <algorithm>

namespace svg {

    using namespace std::literals;

    // ---------- ColorStreamer ------------------
    void ColorStreamer::operator()(std::monostate) const {
        os << "none"s;
    }

    void ColorStreamer::operator()(const std::string& color) const {
        os << color;
    }

    void ColorStreamer::operator()(const Rgb& color) const {
        os << "rgb("sv << static_cast<int>(color.red) << ","sv << static_cast<int>(color.green) << ","sv << static_cast<int>(color.blue) << ")"sv;
    }

    void ColorStreamer::operator()(const Rgba& color) const {
        os << "rgba("sv << static_cast<int>(color.red) << ","sv << static_cast<int>(color.green) << ","sv << static_cast<int>(color.blue) << ","sv << color.opacity << ")"sv;
    }

    std::ostream& operator<<(std::ostream& os, const Color& color) {
        std::visit(ColorStreamer{os}, color);
        return os;
    }

    // ---------- StrokeLineCap ------------------
    std::ostream& operator<<(std::ostream& os, const StrokeLineCap& line_cap) {
        switch (line_cap) {
        case StrokeLineCap::BUTT:
            os << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            os << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            os << "square"sv;
        }
        return os;
    }

    // ---------- StrokeLineJoin ------------------
    std::ostream& operator<<(std::ostream& os, const StrokeLineJoin& line_join) {
        switch (line_join) {
        case StrokeLineJoin::ARCS:
            os << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            os << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            os << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            os << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            os << "round"sv;
        }
        return os;
    }

    // ---------- RenderContext ------------------
    RenderContext RenderContext::Indented() const {
        return { out, indent_step, indent + indent_step };
    }

    void RenderContext::RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // ---------- Object ------------------
    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();
        RenderObject(context);
        context.out << std::endl;
    }

    // ---------- Circle ------------------
    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\" "sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Polyline ------------------
    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(std::move(point));
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;

        out << "<polyline points=\""sv;
        if (!points_.empty()) {
            out << points_[0].x << ","sv << points_[0].y;
            std::for_each(points_.begin() + 1, points_.end(), [&](const auto& p) {out << " "sv << p.x << ","sv << p.y; });
        }
        
        out << "\""sv;
        RenderAttrs(out);
        out << " />"sv;
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
        size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = std::move(data);
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        
        out << "<text x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" "sv;
        out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
        out << "font-size=\""sv << size_ << "\""sv;
        if (font_family_) {
            out << " font-family=\""sv << font_family_.value() << "\""sv;
        }
        if (font_weight_) {
            out << " font-weight=\""sv << font_weight_.value() << "\""sv;
        }
        RenderAttrs(out);
        out << ">"sv << data_ <<"</text>";
    }

    // ---------- Document ------------------
    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.push_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
        RenderContext ctx(out, 2, 2);
        std::for_each(objects_.begin(), objects_.end(), [&](const auto& obj) { obj->Render(ctx); });
        out << "</svg>"sv;
    }
}  // namespace svg