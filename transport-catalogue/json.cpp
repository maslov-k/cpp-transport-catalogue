#include "json.h"

using namespace std;

namespace json
{

namespace
{
using Number = std::variant<int, double>;

Node LoadNode(istream& input);

Node LoadNumber(std::istream& input)
{
	Number number;
	using namespace std::literals;

	string parsed_num;

	// Считывает в parsed_num очередной символ из input
	auto read_char = [&parsed_num, &input]
	{
		parsed_num += static_cast<char>(input.get());
		if (!input)
		{
			throw ParsingError("Failed to read number from stream"s);
		}
	};

	// Считывает одну или более цифр в parsed_num из input
	auto read_digits = [&input, read_char]
	{
		if (!isdigit(input.peek()))
		{
			throw ParsingError("A digit is expected"s);
		}
		while (isdigit(input.peek()))
		{
			read_char();
		}
	};

	if (input.peek() == '-')
	{
		read_char();
	}
	// Парсим целую часть числа
	if (input.peek() == '0')
	{
		read_char();
		// После 0 в JSON не могут идти другие цифры
	}
	else
	{
		read_digits();
	}

	bool is_int = true;
	// Парсим дробную часть числа
	if (input.peek() == '.')
	{
		read_char();
		read_digits();
		is_int = false;
	}

	// Парсим экспоненциальную часть числа
	if (int ch = input.peek(); ch == 'e' || ch == 'E')
	{
		read_char();
		if (ch = input.peek(); ch == '+' || ch == '-')
		{
			read_char();
		}
		read_digits();
		is_int = false;
	}

	try
	{
		if (is_int)
		{
			// Сначала пробуем преобразовать строку в int
			try
			{
				number =  stoi(parsed_num);
			}
			catch (...)
			{
				// В случае неудачи, например, при переполнении,
				// код ниже попробует преобразовать строку в double
			}
		}
		else
		{
			number = stod(parsed_num);
		}
		
	}
	catch (...)
	{
		throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
	}

	if (holds_alternative<int>(number))
	{
		return Node(get<int>(number));
	}
	else if (holds_alternative<double>(number))
	{
		return Node(get<double>(number));
	}
	else
	{
		throw ParsingError("Failed to load number"s);
	}
}

Node LoadArray(istream& input)
{
	char c;
	if (!input.get(c))
	{
		throw ParsingError("Failed to read array from stream"s);
	}
	input.putback(c);
	Array result;

	for (char c; input >> c && c != ']';)
	{
		if (c != ',')
		{
			input.putback(c);
		}
		result.push_back(LoadNode(input));
	}

	return Node(move(result));
}

Node LoadString(istream& input)
{
	string str;
	char c = input.get();
	while (c != '"')
	{
		if (c == '\n' || c == '\r')
		{
			throw ParsingError("Failed to read string from stream"s);
		}
		else if (c != '\\')
		{
			str += c;
		}
		else
		{
			c = input.get();
			switch (c)
			{
			case 'n':
				str += '\n';
				break;
			case '"':
				str += '\"';
				break;
			case 'r':
				str += "\r";
				break;
			case 't':
				str += "\t";
				break;
			case '\\':
				str += "\\";
				break;
			default:
				break;
			}
		}
		if (!input.get(c))
		{
			throw ParsingError("Failed to read string from stream"s);
		}
	}
	return Node(move(str));
}

Node LoadDict(istream& input)
{
	char c;
	if (!input.get(c))
	{
		throw ParsingError("Failed to read array from stream"s);
	}
	input.putback(c);
	Dict result;

	for (char c; input >> c && c != '}';)
	{
		if (c == ',')
		{
			input >> c;
		}

		string key = LoadString(input).AsString();
		input >> c;
		result.insert({ move(key), LoadNode(input) });
	}

	return Node(move(result));
}

Node LoadBoolNull(istream& input)
{
	string parsed_value;
	auto read_char = [&parsed_value, &input]
	{
		parsed_value += static_cast<char>(input.get());
		if (!input)
		{
			throw ParsingError("Failed to read number from stream"s);
		}
	};
	while (isalpha(input.peek()))
	{
		read_char();
	}
	if (parsed_value == "true"s)
	{
		return Node(true);
	}
	else if (parsed_value == "false"s)
	{
		return Node(false);
	}
	else if (parsed_value == "null"s)
	{
		return Node();
	}
	else
	{
		throw ParsingError("Failed to read value from stream"s);
	}
}

Node LoadNode(istream& input)
{
	char c;
	input >> c;

	if (c == '[')
	{
		return LoadArray(input);
	}
	else if (c == '{')
	{
		return LoadDict(input);
	}
	else if (c == '"')
	{
		return LoadString(input);
	}
	else if (c == 't' || c == 'f' || c == 'n')
	{
		input.putback(c);
		return LoadBoolNull(input);
	}
	else if (isdigit(c) || c == '-')
	{
		input.putback(c);
		return LoadNumber(input);
	}
	else
	{
		throw ParsingError("Failed to read value from stream"s);
	}
}

}  // namespace

bool Node::operator==(const Node& rhs) const
{
	return this->node_json_ == rhs.node_json_;
}

bool Node::operator!=(const Node& rhs) const
{
	return !(*this == rhs);
}

const Array& Node::AsArray() const
{
	if (IsArray())
	{
		return get<Array>(node_json_);
	}
	else
	{
		throw logic_error("wrong type"s);
	}
}

const Dict& Node::AsMap() const
{
	if (IsMap())
	{
		return get<Dict>(node_json_);
	}
	else
	{
		throw logic_error("wrong type"s);
	}
}

bool Node::AsBool() const
{
	if (IsBool())
	{
		return get<bool>(node_json_);
	}
	else
	{
		throw logic_error("wrong type"s);
	}
}

int Node::AsInt() const
{
	if (IsInt())
	{
		return get<int>(node_json_);
	}
	else
	{
		throw logic_error("wrong type"s);
	}
}

double Node::AsDouble() const
{
	if (IsInt())
	{
		return get<int>(node_json_);
	}
	else if (IsDouble())
	{
		return get<double>(node_json_);
	}
	else
	{
		throw logic_error("wrong type"s);
	}
}

const string& Node::AsString() const
{
	if (IsString())
	{
		return get<string>(node_json_);
	}
	else
	{
		throw logic_error("wrong type"s);
	}
}

bool Node::IsNull() const
{
	return holds_alternative<nullptr_t>(node_json_);
}

bool Node::IsArray() const
{
	return holds_alternative<Array>(node_json_);
}

bool Node::IsMap() const
{
	return holds_alternative<Dict>(node_json_);
}

bool Node::IsBool() const
{
	return holds_alternative<bool>(node_json_);
}

bool Node::IsInt() const
{
	return holds_alternative<int>(node_json_);
}

bool Node::IsDouble() const
{
	return holds_alternative<double>(node_json_) || holds_alternative<int>(node_json_);
}

bool Node::IsPureDouble() const
{
	return holds_alternative<double>(node_json_);
}

bool Node::IsString() const
{
	if (holds_alternative<string>(node_json_))
	{
		return true;
	}
	return false;
}

NodeJSON Node::GetNode() const
{
	return node_json_;
}

Document::Document(Node root)
	: root_(move(root))
{
}

bool Document::operator==(const Document& rhs) const
{
	return root_ == rhs.root_;
}

bool Document::operator!=(const Document& rhs) const
{
	return !(*this == rhs);
}

const Node& Document::GetRoot() const
{
	return root_;
}

void NodePrinter::operator()(nullptr_t) const
{
	out << "null"s;
}

void NodePrinter::operator()(Array array) const
{
	out << "["s;
	bool is_first = true;
	for (const Node& element : array)
	{
		if (is_first)
		{
			out << "\n"s << string(cur_indent + indent_value, ' ');
			is_first = false;
		}
		else
		{
			out << ","s << "\n"s << string(cur_indent + indent_value, ' ');
		}
		visit(NodePrinter{ out, cur_indent + indent_value }, element.GetNode());
	}
	out << "\n"s << string(cur_indent, ' ') << "]"s;
}

void NodePrinter::operator()(Dict dict) const
{
	out << string(cur_indent, ' ') << "{"s;
	bool is_first = true;
	for (const auto& [key, value] : dict)
	{
		if (is_first)
		{
			out << "\n"s << string(cur_indent + indent_value, ' ') << "\""s << key << "\": ";
			is_first = false;
		}
		else
		{
			out << ","s << "\n"s << string(cur_indent + indent_value, ' ') << "\""s << key << "\": ";
		}
		visit(NodePrinter{ out, cur_indent + indent_value }, value.GetNode());
	}
	out << "\n"s << string(cur_indent, ' ') << "}"s;
}

void NodePrinter::operator()(bool value) const
{
	out << boolalpha << value;
}

void NodePrinter::operator()(int value) const
{
	out << value;
}

void NodePrinter::operator()(double value) const
{
	out << value;
}

void NodePrinter::operator()(string str) const
{
	out << '"';
	for (char c : str)
	{
		switch (c)
		{
		case '\n':
			out << '\\' << 'n';
			break;
		case '\r':
			out << '\\' << 'r';
			break;
		case '\t':
			out << '\\' << 't';
			break;
		case '\\':
			out << '\\' << '\\';
			break;
		case '\"':
			out << '\\' << '"';
			break;
		default:
			out << c;
		}
	}
	out << '"';
}

Document Load(istream& input)
{
	return Document{ LoadNode(input) };
}

void Print(const Document& doc, std::ostream& output, int cur_indent)
{
	NodeJSON node_json = doc.GetRoot().GetNode();
	visit(NodePrinter{ output, cur_indent }, node_json);
}

}  // namespace json