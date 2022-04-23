#include "json_builder.h"

#include <algorithm>

using namespace std;
using namespace json;

KeyItemContext Builder::Key(string key)
{
	if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap())
	{
		throw logic_error("Key error"s);
	}
	Dict& dict = get<Dict>(nodes_stack_.back()->GetValue());
	nodes_stack_.push_back(&dict[move(key)]);
	return *this;
}

BaseContext Builder::Value(Node value)
{
	if (nodes_stack_.empty())
	{
		root_ = Node(value);
		nodes_stack_.push_back(&root_);
		return *this;
	}
	if (!nodes_stack_.back()->IsArray() && !nodes_stack_.back()->IsNull())
	{
		throw logic_error("Value error"s);
	}
	if (nodes_stack_.back()->IsNull())
	{
		*nodes_stack_.back() = move(value);
		nodes_stack_.pop_back();
		return static_cast<ValueContext>(*this);
	}
	else if (nodes_stack_.back()->IsArray())
	{
		Array& arr = get<Array>(nodes_stack_.back()->GetValue());
		arr.push_back(move(value));
		return static_cast<ValueArrayContext>(*this);
	}
	return *this;
}

DictItemContext Builder::StartDict()
{
	if (!nodes_stack_.empty() && !nodes_stack_.back()->IsArray() && !nodes_stack_.back()->IsNull())
	{
		throw logic_error("StartDict error"s);
	}
	if (nodes_stack_.empty())
	{
		root_ = Node(Dict());
		nodes_stack_.push_back(&root_);
	}
	else if (nodes_stack_.back()->IsArray())
	{
		Array& arr = get<Array>(nodes_stack_.back()->GetValue());
		arr.emplace_back(Node(Dict()));
		nodes_stack_.push_back(&arr.back());
	}
	else if (nodes_stack_.back()->IsNull())
	{
		*nodes_stack_.back() = Node(Dict());
	}
	return *this;
}

BaseContext Builder::EndDict()
{
	if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap())
	{
		throw logic_error("EndDict error"s);
	}
	nodes_stack_.pop_back();
	return *this;
}

ArrayItemContext Builder::StartArray()
{
	if (!nodes_stack_.empty() && !nodes_stack_.back()->IsArray() && !nodes_stack_.back()->IsNull())
	{
		throw logic_error("StartArray error"s);
	}
	if (nodes_stack_.empty())
	{
		root_ = Node(Array());
		nodes_stack_.push_back(&root_);
	}
	else if (nodes_stack_.back()->IsArray())
	{
		Array& arr = get<Array>(nodes_stack_.back()->GetValue());
		arr.emplace_back(Node(Array()));
		nodes_stack_.push_back(&arr.back());
	}
	else if (nodes_stack_.back()->IsNull())
	{
		*nodes_stack_.back() = Node(Array());
	}
	return *this;
}

BaseContext Builder::EndArray()
{
	if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray())
	{
		throw logic_error("EndArray error"s);
	}
	nodes_stack_.pop_back();
	return *this;
}

Node Builder::Build()
{
	if ((nodes_stack_.empty() && root_.IsNull()) ||
		(!nodes_stack_.empty() && nodes_stack_.back() != &root_))
	{
		throw logic_error("bilding error"s);
	}
	return root_;
}

BaseContext::BaseContext(Builder& builder)
	: builder_(builder)
{
}

KeyItemContext BaseContext::Key(string key)
{
	return builder_.Key(move(key));
}

BaseContext BaseContext::Value(Node value)
{
	return builder_.Value(move(value));
}

DictItemContext BaseContext::StartDict()
{
	return builder_.StartDict();
}

BaseContext BaseContext::EndDict()
{
	return builder_.EndDict();
}

ArrayItemContext BaseContext::StartArray()
{
	return builder_.StartArray();
}

BaseContext BaseContext::EndArray()
{
	return builder_.EndArray();
}

Node BaseContext::Build()
{
	return builder_.Build();
}

KeyItemContext::KeyItemContext(Builder& builder)
	: BaseContext(builder)
{
}

ValueContext KeyItemContext::Value(Node value)
{
	return static_cast<ValueContext>(BaseContext::Value(move(value)));
}

DictItemContext::DictItemContext(Builder& builder)
	: BaseContext(builder)
{
}

ArrayItemContext::ArrayItemContext(Builder& builder)
	: BaseContext(builder)
{
}

ValueArrayContext ArrayItemContext::Value(Node value)
{
	return static_cast<ValueArrayContext>(BaseContext::Value(move(value)));
}

ValueContext::ValueContext(BaseContext base)
	: BaseContext(base)
{
}

ValueArrayContext::ValueArrayContext(BaseContext base)
	: BaseContext(base)
{
}

ValueArrayContext ValueArrayContext::Value(Node value)
{
	return static_cast<ValueArrayContext>(BaseContext::Value(move(value)));
}
