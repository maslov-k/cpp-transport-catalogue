#pragma once

#include "json.h"

namespace json
{

class BaseContext;
class DictItemContext;
class KeyItemContext;
class ArrayItemContext;
class ValueContext;
class ValueArrayContext;

class Builder
{
public:
	KeyItemContext Key(std::string key);
	BaseContext Value(Node value);
	DictItemContext StartDict();
	BaseContext EndDict();
	ArrayItemContext StartArray();
	BaseContext EndArray();
	Node Build();

private:
	Node root_;
	std::vector<Node*> nodes_stack_;
};

class BaseContext
{
public:
	BaseContext(Builder& builder);

	KeyItemContext Key(std::string key);
	BaseContext Value(Node value);
	DictItemContext StartDict();
	BaseContext EndDict();
	ArrayItemContext StartArray();
	BaseContext EndArray();
	Node Build();

private:
	Builder& builder_;
};

class KeyItemContext : public BaseContext
{
public:
	KeyItemContext(Builder& builder);

	KeyItemContext Key(std::string key) = delete;
	BaseContext EndArray() = delete;
	BaseContext EndDict() = delete;
	Node Build() = delete;

	ValueContext Value(Node value);
};

class DictItemContext : public BaseContext
{
public:
	DictItemContext(Builder& builder);

	BaseContext Value(Node value) = delete;
	DictItemContext StartDict() = delete;
	ArrayItemContext StartArray() = delete;
	BaseContext EndArray() = delete;
	Node Build() = delete;
};

class ArrayItemContext : public BaseContext
{
public:
	ArrayItemContext(Builder& builder);

	KeyItemContext Key(std::string key) = delete;
	BaseContext EndDict() = delete;
	Node Build() = delete;

	ValueArrayContext Value(Node value);
};

class ValueContext : public BaseContext
{
public:
	ValueContext(BaseContext base);

	BaseContext Value(Node value) = delete;
	DictItemContext StartDict() = delete;
	ArrayItemContext StartArray() = delete;
	BaseContext EndArray() = delete;
	Node Build() = delete;
};

class ValueArrayContext : public BaseContext
{
public:
	ValueArrayContext(BaseContext base);

	KeyItemContext Key(std::string key) = delete;
	BaseContext EndDict() = delete;
	Node Build() = delete;

	ValueArrayContext Value(Node value);
};

}
