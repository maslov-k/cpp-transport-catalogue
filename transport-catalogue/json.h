#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json
{

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;
using NodeJSON = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error
{
public:
	using runtime_error::runtime_error;
};

class Node
{
public:
	explicit Node() = default;

	template <typename Type>
	Node(Type value);

	bool operator==(const Node& rhs) const;
	bool operator!=(const Node& rhs) const;

	const Array& AsArray() const;
	const Dict& AsMap() const;
	bool AsBool() const;
	int AsInt() const;
	double AsDouble() const;
	const std::string& AsString() const;

	bool IsNull() const;
	bool IsArray() const;
	bool IsMap() const;
	bool IsBool() const;
	bool IsInt() const;
	bool IsDouble() const;
	bool IsPureDouble() const;
	bool IsString() const;

	NodeJSON GetNode() const;

private:
	NodeJSON node_json_;
};

class Document
{
public:
	explicit Document(Node root);

	bool operator==(const Document& rhs) const;
	bool operator!=(const Document& rhs) const;

	const Node& GetRoot() const;

private:
	Node root_;
};

struct NodePrinter
{
	std::ostream& out;
	int cur_indent = 0;
	const int indent_value = 4;

	void operator()(nullptr_t) const;
	void operator()(Array array) const;
	void operator()(Dict dict) const;
	void operator()(bool value) const;
	void operator()(int value) const;
	void operator()(double value) const;
	void operator()(std::string) const;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output, int cur_indent = 0);

template<typename Type>
Node::Node(Type value)
	: node_json_(std::move(value))
{
}

}  // namespace json