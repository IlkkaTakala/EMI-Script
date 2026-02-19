#include <EMI/EMI.h>
#include <thread>
#include <chrono>
#include <numeric>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <vector>
#include <string>

using namespace std::literals::chrono_literals;

template <typename Out>
void split(const std::string& s, char delim, Out result) {
	std::istringstream iss(s);
	std::string item;
	while (std::getline(iss, item, delim)) {
		*result++ = item;
	}
}

std::vector<std::string> split(const std::string& s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}

// Parse arguments with support for quoted strings and simple escapes
static std::vector<std::string> parseArgs(const std::string& s) {
	std::vector<std::string> out;
	std::string cur;
	bool inQuote = false;
	char quoteChar = '\0';
	for (size_t i = 0; i < s.size(); ++i) {
		char c = s[i];
		if (!inQuote) {
			if (std::isspace((unsigned char)c)) {
				if (!cur.empty()) { out.push_back(cur); cur.clear(); }
				continue;
			}
			if (c == '"' || c == '\'') {
				inQuote = true; quoteChar = c; continue;
			}
			if (c == '\\' && i + 1 < s.size()) {
				cur.push_back(s[++i]); continue;
			}
			cur.push_back(c);
		} else {
			if (c == quoteChar) {
				inQuote = false; quoteChar = '\0';
				continue;
			}
			if (c == '\\' && i + 1 < s.size()) {
				cur.push_back(s[++i]); continue;
			}
			cur.push_back(c);
		}
	}
	if (!cur.empty()) out.push_back(cur);
	return out;
}

// Command registry types
struct CommandContext {
	EMI::VMHandle& vm;
};

struct Command {
	std::string name;
	std::vector<std::string> aliases;
	std::string usage;
	std::string description;
	std::function<int(CommandContext&, const std::vector<std::string>&)> handler;
};

struct CommandRegistry {
	std::vector<Command> commands;
	std::unordered_map<std::string, size_t> lookup;

	void Register(Command cmd) {
		size_t idx = commands.size();
		commands.push_back(std::move(cmd));
		const auto& c = commands.back();
		lookup.emplace(c.name, idx);
		for (const auto& a : c.aliases) lookup.emplace(a, idx);
	}

	const Command* Find(const std::string& name) const {
		auto it = lookup.find(name);
		if (it == lookup.end()) return nullptr;
		return &commands[it->second];
	}

	const std::vector<Command>& All() const { return commands; }
};

int ScriptingMode(EMI::VMHandle& vm)
{
	printf("\nRunning in scripting mode\n");
	std::string line;
	while (true)
	{
		printf("\n>> ");
		std::getline(std::cin, line);
		if (line == "quit") {
			break;
		} else if (line == "step") {
			vm.Step();
		} else if (line == "up") {
			vm.StepUp();
		} else if (line == "down") {
			vm.StepDown();
		} else if (line == "resume") {
			vm.Resume();
		} else if (line == "pause") {
			vm.Pause();
		}
		else {
			vm.CompileTemporary(line.c_str());
		}
	}
	printf("Exiting script stage\n\n");

	return 0;
}

int main(int argc, char** argv)
{
	srand((unsigned)time(0));

	CommandRegistry registry;

	// Register commands (handlers receive a context so they can access vm at runtime)
	registry.Register(Command{
		"exit",
		{},
		"exit",
		"Exit the CLI",
		[](CommandContext& , const std::vector<std::string>&) {
			// signal graceful shutdown
			return -1;
		}
	});

	registry.Register(Command{
		"compile",
		{},
		"compile <paths...>",
		"Compile script files",
		[](CommandContext& ctx, const std::vector<std::string>& params) {
			static std::unordered_map<std::string, std::function<void(EMI::Options&, std::vector<int>&, const std::vector<std::string>&)>> optionMap = {
				{"-b", [](EMI::Options& options, std::vector<int>& breaks, const std::vector<std::string>& args) { for (const auto& a : args) { try { breaks.push_back(std::stoi(a)); } catch (...) {} } }}
			};
			std::vector<std::string> path;
			std::vector<int> breaks;
			EMI::Options options;
			bool sawDash = false;
			for (size_t i = 1; i < params.size(); ++i) {
				const std::string& p = params[i];
				if (!sawDash) {
					if (!p.empty() && p[0] == '-') {
						sawDash = true;
					}
					else {
						path.push_back(p);
						continue;
					}
				}

				auto it = optionMap.find(p);
				std::vector<std::string> values;
				size_t j = i + 1;
				for (; j < params.size(); ++j) {
					if (!params[j].empty() && params[j][0] == '-') break;
					values.push_back(params[j]);
				}
				if (it != optionMap.end()) {
					it->second(options, breaks, values);
				}
				i = j - 1;
			}
			bool result = false;
			options.BreakpointCount = breaks.size();
			options.Breakpoints = breaks.data();
			for (const auto& p : path) {
				auto handle = ctx.vm.CompileScript(p.c_str(), options);
				result |= handle.wait();
			}
			return result ? 1 : 0;
		}
	});

	registry.Register(Command{
		"reinit",
		{},
		"reinit",
		"Reinitialize grammar",
		[](CommandContext& ctx, const std::vector<std::string>&) {
			ctx.vm.ReinitializeGrammar("../../.grammar");
			return 0;
		}
	});

	registry.Register(Command{
		"emi",
		{},
		"emi",
		"Enter scripting mode",
		[](CommandContext& ctx, const std::vector<std::string>&) { return ScriptingMode(ctx.vm); }
	});

	registry.Register(Command{
		"loglevel",
		{},
		"loglevel <n>",
		"Set global log level",
		[](CommandContext& , const std::vector<std::string>& params) { if (params.size() > 1) EMI::SetLogLevel((EMI::LogLevel)std::atoi(params[1].c_str())); return 0; }
	});

	registry.Register(Command{
		"export",
		{},
		"export <file>",
		"Export VM to file",
		[](CommandContext& ctx, const std::vector<std::string>& params) { 
			if (params.size() > 1) {
				ctx.vm.ExportVM(params[1].c_str());
			}
			return 0;
		}
	});

	registry.Register(Command{
		"call",
		{},
		"call <func> [arg]",
		"Call a VM function with optional string arg",
		[](CommandContext& ctx, const std::vector<std::string>& params) {
			if (params.size() == 3) {
				auto handle = ctx.vm.GetFunctionHandle(params[1].c_str());
				handle(params[2].c_str());
			}
			if (params.size() == 2) {
				auto handle = ctx.vm.GetFunctionHandle(params[1].c_str());
				handle();
			}
			return 0;
		}
	});

	// help command
	registry.Register(Command{
		"help",
		{"?"},
		"help [command]",
		"Show help for commands",
		[&registry](CommandContext& , const std::vector<std::string>& params) {
			if (params.size() > 1) {
				if (const Command* c = registry.Find(params[1])) {
					std::cout << c->name << "\n" << c->usage << "\n" << c->description << "\n";
				}
				else std::cout << "Unknown command: " << params[1] << '\n';
				return 0;
			}
			for (const auto& c : registry.All()) {
				std::cout << c.name << " - " << c.description << "\n";
			}
			return 0;
		}
	});

	EMI::SetLogLevel(EMI::LogLevel::Debug);
	EMI::SetScriptLogLevel(EMI::LogLevel::Info);
	auto vm = EMI::CreateEnvironment();
	vm.ReinitializeGrammar("../../.grammar");

	CommandContext ctx{ vm };

	// Modes:
	// - no argv (argc == 1): enter scripting mode
	// - argv[1] == "internal": use interactive CLI (current behaviour)
	// - otherwise: treat argv[1..] as a single command to run and exit

	if (argc == 1) {
		return ScriptingMode(vm);
	}

	std::string firstArg = argv[1];
	if (firstArg == "internal") {
		// interactive CLI (existing behaviour)
		bool run = true;
		std::string input;
		while (run) {
			printf("\nemi: ");
			std::getline(std::cin, input);
			auto res = parseArgs(input);
			if (res.size() < 1) continue;

			int result = 0;
			if (const Command* cmd = registry.Find(res[0])) {
				result = cmd->handler(ctx, res);
			}

			if (result == 0) continue;

			if (result == -1) { // exit
				break;
			}
		}

		EMI::ReleaseEnvironment(vm);
		std::this_thread::sleep_for(10ms);
		return 0;
	}

	// Otherwise, run a single command from argv
	std::vector<std::string> params;
	for (int i = 1; i < argc; ++i) params.emplace_back(argv[i]);

	if (const Command* cmd = registry.Find(params[0])) {
		cmd->handler(ctx, params);

		EMI::ReleaseEnvironment(vm);
		std::this_thread::sleep_for(10ms);
		return 0;
	}

	std::cerr << "Unknown command: " << params[0] << '\n';
	EMI::ReleaseEnvironment(vm);
	std::this_thread::sleep_for(10ms);
	return 1;
}

