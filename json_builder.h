#pragma once
#include "json.h"
#include <vector>
#include <memory>


namespace json {
	class DictItemBuilder;
	class DictValueBuilder;
	class ArrayItemBuilder;

	class Builder {
	public:
		Builder() = default;

		Builder& Value(const Node::Value& value);
		DictValueBuilder Key(const std::string& key);
		ArrayItemBuilder StartArray();
		Builder& EndArray();
		DictItemBuilder StartDict();
		Builder& EndDict();
		const Node& Build();

	private:
		Node root_;
		std::vector<std::unique_ptr<Node>> nodes_stack_;
		std::vector<size_t> arrays_;
		std::vector<size_t> dicts_;

		bool is_key = false;

		std::unique_ptr<Node> ConstractNodePtr(Node::Value value);
	};

	class BaseBuilder {
	public:
		explicit BaseBuilder(Builder& builder) : builder_(builder) {}
		DictItemBuilder StartDict();
		ArrayItemBuilder StartArray();
	protected:
		Builder& builder_;
	};

	class DictValueBuilder : public BaseBuilder {
	public:
		explicit DictValueBuilder(Builder& builder) : BaseBuilder(builder) {}
		DictItemBuilder Value(const Node::Value& value);	
	};

	class DictItemBuilder : public BaseBuilder {
	public:
		explicit DictItemBuilder(Builder& builder) : BaseBuilder(builder) {}
		DictValueBuilder Key(const std::string& key);
		Builder& EndDict();

		DictItemBuilder StartDict() = delete;
		ArrayItemBuilder StartArray() = delete;
	};

	class ArrayItemBuilder : public BaseBuilder {
	public:
		explicit ArrayItemBuilder(Builder& builder) : BaseBuilder(builder) {}
		ArrayItemBuilder Value(const Node::Value& value);
		Builder& EndArray();
	};
	
}

