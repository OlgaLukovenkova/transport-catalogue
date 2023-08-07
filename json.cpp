#include "json.h"

using namespace std;

namespace json {

    bool Node::IsInt() const {
        return holds_alternative<int>(*this);
    }

    bool Node::IsDouble() const {
        return holds_alternative<int>(*this) || holds_alternative<double>(*this);
    }

    bool Node::IsPureDouble() const {
        return holds_alternative<double>(*this);
    }

    bool Node::IsBool() const {
        return holds_alternative<bool>(*this);
    }

    bool Node::IsString() const {
        return holds_alternative<string>(*this);
    }

    bool Node::IsNull() const {
        return holds_alternative<nullptr_t>(*this);
    }

    bool Node::IsArray() const {
        return holds_alternative<Array>(*this);
    }

    bool Node::IsMap() const {
        return holds_alternative<Dict>(*this);
    }

    int Node::AsInt() const {
        return As<int>();
    }

    bool Node::AsBool() const {
        return As<bool>();
    }

    double Node::AsDouble() const {
        try {
            return As<int>();
        }
        catch (const logic_error&) {
            return As<double>();
        }
    }

    const string& Node::AsString() const {
        return As<string>();
    }

    const Array& Node::AsArray() const {
        return As<Array>();
    }

    const Dict& Node::AsMap() const {
        return As<Dict>();
    }

    const Node::Value& Node::GetValue() const {
        return *this;
    }

    bool operator==(const Node& node1, const Node& node2) {
        return static_cast<Node::Value>(node1) == static_cast<Node::Value>(node2);
    }

    bool operator!=(const Node& node1, const Node& node2) {
        return !(node1 == node2);
    }

    // ------- Document -----------------------
    Document::Document(Node root)
        : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(istream& input) {
        return Document{ detail::LoadNode(input) };
    }

    void Print(const Document& doc, std::ostream& output) {
        detail::PrintContext ctx{ output, 4, 0 };
        detail::PrintNode(doc.GetRoot(), output, ctx);
    }

    namespace detail {

        Node LoadArray(istream& input) {
            Array result;

            for (char c = ' '; input >> c && c != ']';) {

                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }

            if (!input) {
                throw ParsingError("Array ended before \"]\"");
            }

            return Node(move(result));
        }

        Node LoadString(istream& input) {
            string line;

            while (true) {
                // if nothing to read or new line or carriage return
                if (!input || input.peek() == '\n' || input.peek() == '\r') {
                    throw ParsingError("String parsing error"s);
                }
                // if meet ")" 
                if (input.peek() == '\"') {
                    input.get();
                    break;
                }

                // if meet escapable sequence
                if (input.peek() == '\\') {
                    input.get();

                    switch (static_cast<char>(input.get())) {
                    case 'n':
                        line.push_back('\n');
                        break;
                    case 'r':
                        line.push_back('\r');
                        break;
                    case 't':
                        line.push_back('\t');
                        break;
                    case '\\':
                        line.push_back('\\');
                        break;
                    case '"':
                        line.push_back('\"');
                        break;
                    default:
                        throw ParsingError("String parsing error"s);
                    }
                }
                else {
                    line.push_back(static_cast<char>(input.get()));
                }
            }

            return Node(move(line));
        }

        Node LoadDict(istream& input) {
            Dict result;

            for (char c; input >> c && c != '}';) {
                if (c == ',') {
                    input >> c;
                }

                string key = LoadString(input).AsString();
                input >> c; // sign :
                result.insert({ move(key), LoadNode(input) });
            }

            if (!input) {
                throw ParsingError("Array ended before \"]\"");
            }

            return Node(move(result));
        }

        Node LoadNumber(istream& input) {
            string num = ""s;
            bool is_int = true;

            auto read_digits = [&num, &input]() {
                if (!isdigit(input.peek())) {
                    throw ParsingError("A digit was expected"s);
                }
                while (isdigit(input.peek())) {
                    num += static_cast<char>(input.get());
                }
            };

            // sign
            if (input.peek() == '-') {
                num += static_cast<char>(input.get());
            }

            // or 0
            if (input.peek() == '0') {
                num += static_cast<char>(input.get());
            }
            // or digital sequence (not starts with 0)
            else {
                read_digits();
            }

            //if '.' then float number
            if (input.peek() == '.') {
                num += static_cast<char>(input.get());
                is_int = false;
                read_digits();
            }

            // if have 'E' or 'e'
            if (tolower(input.peek()) == 'e') {
                num += static_cast<char>(input.get());
                // after 'E' or 'e' should be sign
                if (input.peek() == '+' || input.peek() == '-') {
                    num += static_cast<char>(input.get());
                }
                is_int = false;
                read_digits();
            }

            try {
                if (is_int) {
                    try {
                        int value = stoi(num);
                        return Node{ value };
                    }
                    catch (...) {

                    }
                }
                double value = stod(num);
                return Node{ value };
            }
            catch (...) {
                throw ParsingError(num + " could not be converted"s);
            }
        }

        Node LoadNull(istream& input) {
            char null_str[5];
            input.get(null_str, 5);
            null_str[4] = '\0';
            if (string(null_str) == "null"s) {
                return { nullptr };
            }
            throw ParsingError("\"null\" was expected");

        }

        Node LoadBool(istream& input) {
            char null_str[6];
            input.get(null_str, 5);
            null_str[4] = '\0';
            if (string(null_str) == "true"s) {
                return { true };
            }
            null_str[4] = static_cast<char>(input.get());
            null_str[5] = '\0';
            if (string(null_str) == "false"s) {
                return { false };
            }
            throw ParsingError("\"true\" or \"false\" were expected");
        }

        Node LoadNode(istream& input) {
            char c;
            input >> c;

            if (c == '[') {
                return LoadArray(input);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == '"') {
                return LoadString(input);
            }
            else if (c == 'n') {
                input.putback(c);
                return LoadNull(input);
            }
            else if (c == 't' || c == 'f') {
                input.putback(c);
                return LoadBool(input);
            }
            else {
                input.putback(c);
                return LoadNumber(input);
            }
        }

        void PrintNode(const Node& node, ostream& output, const PrintContext& ctx, bool without_indent_before) {
            visit(
                [&](const auto& value) { PrintValue(value, output, ctx, without_indent_before); },
                node.GetValue());
        }

        void PrintValue(int x, ostream& output, const PrintContext& ctx, bool without_indent_before) {
            if (!without_indent_before) {
                ctx.PrintIndent();
            }
            output << x;
        }

        void PrintValue(double x, ostream& output, const PrintContext& ctx, bool without_indent_before) {
            if (!without_indent_before) {
                ctx.PrintIndent();
            }
            output << x;
        }

        void PrintValue(const string& line, ostream& output, const PrintContext& ctx, bool without_indent_before) {
            if (!without_indent_before) {
                ctx.PrintIndent();
            }
            
            output << "\""s;
            for (char c : line) {
                if (c == '\\') {
                    output << "\\\\"s;
                }
                else if (c == '\n') {
                    output << "\\n"s;
                }
                else if (c == '\r') {
                    output << "\\r"s;
                }
                else if (c == '\"') {
                    output << "\\\""s;
                }
                else {
                    output << c;
                }

            }
                
            output << "\""s;
        }

        void PrintValue(bool x, ostream& output, const PrintContext& ctx, bool without_indent_before) {
            if (!without_indent_before) {
                ctx.PrintIndent();
            }
            output <<  (x ? "true" : "false");
        }

        void PrintValue(nullptr_t, ostream& output, const PrintContext& ctx, bool without_indent_before) {
            if (!without_indent_before) {
                ctx.PrintIndent();
            }
            output << "null";
        }

        void PrintValue(const Array& array, ostream& output, const PrintContext& ctx, bool without_indent_before) {
            if (!without_indent_before) {
                ctx.PrintIndent();
            }
            output << "[\n"s;
            bool is_not_first = false;
            for (const auto& item : array) {
                if (is_not_first) {
                    output << ",\n";
                }
                PrintContext indented_ctx = ctx.Indented();
                PrintNode(item, output, indented_ctx);
                is_not_first = true;
            }
            output << "\n"s;
            ctx.PrintIndent();
            output << "]"s;
        }

        void PrintValue(const Dict& array, ostream& output, const PrintContext& ctx, bool without_indent_before) {
            if (!without_indent_before) {
                ctx.PrintIndent();
            }
            output << "{\n"s;
            bool is_not_first = false;
            for (const auto& [str, node] : array) {
                if (is_not_first) {
                    output << ",\n";
                }
                PrintContext indented_ctx = ctx.Indented();
                indented_ctx.PrintIndent();
                output << "\""s << str << "\" : "s;
                PrintNode(node, output, indented_ctx, true);
                is_not_first = true;
            }
            output << "\n"s;
            ctx.PrintIndent();
            output << "}"s;
        }
    }

}  // namespace json