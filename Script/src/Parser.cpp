#include "Parser.h"
#include <functional>
#include <set>
#include <iostream>

void Parser::ThreadedParse(VM* vm)
{
	while (vm->CompileRunning) {

		std::unique_lock lk(vm->CompileMutex);
		vm->QueueNotify.wait(lk, [vm] {return !vm->CompileQueue.empty() || !vm->CompileRunning; });
		if (!vm->CompileRunning) return;

		auto options = vm->CompileQueue.front();
		vm->CompileQueue.pop();
		lk.unlock();

		Parse(vm, options);
	}
}

struct Data {
	const Token token;
	const std::string_view string;
};

struct Iterator
{
	std::vector<Data>::iterator iter;

	const Data& Peek(int i = 0) {
		return *(iter + i);
	}

	void Pop() {
		iter++;
	}
};

struct Node
{
	std::vector<std::string_view> Values;
	Node* parent;
	std::vector<Node*> children;
	Node* down;

	~Node() {
		for (auto& c : children) delete c;
		delete down;
	}
};

struct Type
{
	bool Compound;
	std::set<int> ID;
	bool TakeValue = false;
	bool Optional = false;
};

struct Rule
{
	std::function<Node*(void)> Spawner;
	bool ReturnResult;
	std::vector<std::vector<Type>> Rules;
	bool Repeatable = false;

	bool Try(Node*& retNode, Iterator& lexer, const std::vector<Rule>& ruleList, const std::set<int>& expected) const {
		
		bool success = false;
		for (const auto& set : Rules) {
			bool setSuccess = true;
			int idx = 0;
			Iterator l = lexer;
			std::vector<Node*> child;
			std::vector<std::string_view> values;
			for (const auto& r : set) {
				if (r.Compound) {
					Node* n;
					auto target = expected;
					for (int i = idx; i < set.size(); i++) if (!set[i].Compound) { target = set[i].ID; break; }
					bool ruleSuccess = false;
					for (auto& id : r.ID) {
						if (ruleList[id].Try(n, l, ruleList, target)) {
							if (r.TakeValue) child.push_back(n);
							ruleSuccess = true;
							break;
						}
					}
					if (!ruleSuccess) {
						setSuccess = false;
						break;
					}
				}
				else {
					auto& d = l.Peek();
					if (r.ID.find((int)d.token) != r.ID.end()) {
						if (r.TakeValue) {
							child.push_back(nullptr);
							values.push_back(d.string);
						}
						l.Pop();
					} 
					else {
						setSuccess = false;
						break;
					}
				}
				idx++;
			}

			lexer = l;
			if (setSuccess && expected.find((int)l.Peek().token) != expected.end()) {
				Node* node = Spawner();
				node->Values = std::move(values);
				node->children = std::move(child);
				if (ReturnResult) retNode = node;
				return true;
			}
		}

		std::cout << "Parse error, unexpected token: " << lexer.Peek().string << '\n';
		return false;
	}
};

std::vector<Rule> Rules = {
	// line
	{[] { return new Node(); }, true, 
	{
		{{true, {1}, true, false},
		{false, {(int)Token::Semi}}}
	}, true},

	// 1 expr
	{[] { return new Node(); }, true, 
	{
		{{false, {(int)Token::Integer}, true}},
		{{false, {(int)Token::Float}, true}},
		{{true, {3}, true}},
		{{true, {2}, true}},
	}},

	// 2 Arith
	{[] { return new Node(); }, true,
	{
		{{true, {1}, true},
		{false, {(int)Token::Add, (int)Token::Sub, (int)Token::Mult, (int)Token::Div}, false},
		{true, {1}, true}},
	}},

	// 3 prio
	{[] { return new Node(); }, true,
	{
		{{false, {(int)Token::Lparenthesis}, false},
		{false, {1}, true},
		{false, {(int)Token::Rparenthesis}, false}},
	}},
};

void Parser::Parse(VM* vm, CompileOptions& options)
{
	Lexer lex(options.Path);

	int count = 0;
	auto token = Token::None;
	std::string_view view;

	std::vector<Data> TokenList;
	TokenList.reserve(100);

	do {
		token = lex.GetNext(view);
		TokenList.emplace_back(token, view);
	} while (token != Token::None);

	Iterator iter(TokenList.begin());
	Node* start;
	Rules[0].Try(start, iter, Rules, { (int)Token::None });
}
