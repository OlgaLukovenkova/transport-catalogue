#include "json_builder.h"

namespace json {

	/*Builder*/
	Builder& Builder::Value(const Node::Value& value) {
		if (nodes_stack_.empty() || !arrays_.empty() || (!dicts_.empty() && is_key)) {
			nodes_stack_.push_back(ConstractNodePtr(value));
			if (!dicts_.empty() && is_key) {
				is_key = false;
			}
			return *this;
		}
		nodes_stack_.clear();
		throw std::logic_error("The value could not be added");
	}

	DictValueBuilder Builder::Key(const std::string& key) {
		if (!dicts_.empty() && !is_key) {
			nodes_stack_.push_back(ConstractNodePtr(key));
			is_key = true;
			return DictValueBuilder{ *this };
		}

		nodes_stack_.clear();
		throw std::logic_error("The key could not be added");
	}

	ArrayItemBuilder Builder::StartArray() {
		if (nodes_stack_.empty() || !arrays_.empty() || (!dicts_.empty() && is_key)) {
			nodes_stack_.push_back( std::make_unique<Node>( Array{} ) );
			arrays_.push_back(nodes_stack_.size()-1);
			return ArrayItemBuilder{ *this };
		}
		nodes_stack_.clear();
		throw std::logic_error("The Array could not be added");
	}

	Builder& Builder::EndArray() {
		if (arrays_.empty()) {
			nodes_stack_.clear();
			throw std::logic_error("The Array could not be ended");
		}

		Array arr;

		while (nodes_stack_.size() > arrays_.back() + 1) {
			arr.insert(arr.begin(), std::move(*nodes_stack_.back()));
			nodes_stack_.pop_back();
		}
		*nodes_stack_.rbegin() = std::make_unique<Node>(std::move(arr));
		arrays_.pop_back();
		is_key = false;
		return *this;
	}

	DictItemBuilder Builder::StartDict() {
		if (nodes_stack_.empty() || !arrays_.empty() || (!dicts_.empty() && is_key) ) {
			nodes_stack_.push_back(std::make_unique<Node>(Dict{}));
			dicts_.push_back(nodes_stack_.size() - 1);
			is_key = false;
			return DictItemBuilder{ *this };
		}
		nodes_stack_.clear();
		throw std::logic_error("The Dict could not be added");
	}

	Builder& Builder::EndDict() {
		if (dicts_.empty()) {
			nodes_stack_.clear();
			throw std::logic_error("The Dict could not be ended");
		}

		Dict dict;

		while (nodes_stack_.size() > dicts_.back() + 1) {
			dict[(*(nodes_stack_.rbegin() + 1))->AsString()] = std::move(*nodes_stack_.back());
			nodes_stack_.pop_back();
			nodes_stack_.pop_back();
		}
		nodes_stack_.pop_back();
		nodes_stack_.push_back(std::make_unique<Node>(std::move(dict)));
		dicts_.pop_back();
		is_key = false;
		return *this;
	}

	const Node& Builder::Build() {
		if (nodes_stack_.empty() || !dicts_.empty() || !arrays_.empty()) {
			throw std::logic_error("Object was not constructed");
		}

		root_ = std::move(*nodes_stack_.back());
		nodes_stack_.clear();
		return root_;
	}

	std::unique_ptr<Node> Builder::ConstractNodePtr(Node::Value value) {
		if (std::holds_alternative<std::nullptr_t>(value)) {
			return std::make_unique<Node>(nullptr);
		}
		if (std::holds_alternative<Array>(value)) {
			return std::make_unique<Node>(std::move(std::get<Array>(value)));
		}
		if (std::holds_alternative<Dict>(value)) {
			return std::make_unique<Node>(std::move(std::get<Dict>(value)));
		}
		if (std::holds_alternative<int>(value)) {
			return std::make_unique<Node>(std::get<int>(value));
		}
		if (std::holds_alternative<bool>(value)) {
			return std::make_unique<Node>(std::get<bool>(value));
		}
		if (std::holds_alternative<double>(value)) {
			return std::make_unique<Node>(std::get<double>(value));
		}
		if (std::holds_alternative<std::string>(value)) {
			return std::make_unique<Node>(std::move(std::get<std::string>(value)));
		}

		return std::make_unique<Node>(nullptr);
	}

	/*BaseBuilder*/
	DictItemBuilder BaseBuilder::StartDict() {
		return builder_.StartDict();
	}

	ArrayItemBuilder BaseBuilder::StartArray() {
		return builder_.StartArray();
	}

	/*DictValueBuilder*/
	DictItemBuilder DictValueBuilder::Value(const Node::Value& value) {
		return DictItemBuilder{ builder_.Value(value) };
	}

	/*DictItemBuilder*/
	DictValueBuilder DictItemBuilder::Key(const std::string& key) {
		return builder_.Key(key);
	}

	Builder& DictItemBuilder::EndDict() {
		return builder_.EndDict();
	}

	/*ArrayItemBuilder*/
	ArrayItemBuilder ArrayItemBuilder::Value(const Node::Value& value) {
		return ArrayItemBuilder{ builder_.Value(value) };
	}

	Builder& ArrayItemBuilder::EndArray() {
		return builder_.EndArray();
	}
}
