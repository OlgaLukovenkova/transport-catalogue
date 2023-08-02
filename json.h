#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    class Node;
    class Document;

    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;
    

    class Node final : private std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict> {
    public:
        using Value = std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>;
        using Value::variant;
        
        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const; 
        bool IsBool() const;
        bool IsString() const;
        bool IsNull() const;
        bool IsArray() const;
        bool IsMap() const;

        int AsInt() const;
        bool AsBool() const;
        double AsDouble() const; 
        const std::string& AsString() const;
        const Array& AsArray() const;
        const Dict& AsMap() const;

        const Value& GetValue() const;

    private:
        template<typename T>
        const T& As() const {
            const T* ptr = std::get_if<T>(this);
            if (!ptr) {
                throw std::logic_error("Wrong type");
            }
            return *ptr;
        }

        friend  bool operator==(const Node& node1, const Node& node2);
    };

    bool operator==(const Node& node1, const Node& node2);
    bool operator!=(const Node& node1, const Node& node2);

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

    private:
        Node root_;
    };

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);

    namespace detail {

        Node LoadNode(std::istream& input);
        Node LoadArray(std::istream& input);
        Node LoadString(std::istream& input);
        Node LoadDict(std::istream& input);
        Node LoadNumber(std::istream& input);
        Node LoadNull(std::istream& input);
        Node LoadBool(std::istream& input);

        struct PrintContext {
            std::ostream& out;
            int indent_step = 4;
            int indent = 0;

            void PrintIndent() const {
                for (int i = 0; i < indent; ++i) {
                    out.put(' ');
                }
            }

            PrintContext Indented() const {
                return { out, indent_step, indent + indent_step };
            }
        };

        void PrintNode(const Node& node, std::ostream& output, const PrintContext& ctx, bool without_indent_before = false);
        void PrintValue(int x, std::ostream& output, const PrintContext& ctx, bool without_indent_before = false);
        void PrintValue(double x, std::ostream& output, const PrintContext& ctx, bool without_indent_before = false);
        void PrintValue(const std::string& line, std::ostream& output, const PrintContext& ctx, bool without_indent_before = false);
        void PrintValue(bool x, std::ostream& output, const PrintContext& ctx, bool without_indent_before = false);
        void PrintValue(std::nullptr_t, std::ostream& output, const PrintContext& ctx, bool without_indent_before = false);
        void PrintValue(const Array& array, std::ostream& output, const PrintContext& ctx, bool without_indent_before = false);
        void PrintValue(const Dict& array, std::ostream& output, const PrintContext& ctx, bool without_indent_before = false);
    } //namespace detail

}  