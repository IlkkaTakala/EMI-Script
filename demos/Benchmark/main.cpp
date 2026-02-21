#include <EMI/EMI.h>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>

static std::string json_escape(const std::string &s) {
    std::ostringstream o;
    for (unsigned char c : s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if (c < 0x20) {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c << std::dec;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

int main()
{
    using namespace std::chrono;
    std::srand((unsigned)time(nullptr));
    std::cout << "EMI Script Benchmark\n";

    EMI::SetLogLevel(EMI::LogLevel::Error);

    std::filesystem::path scriptsDir = "Scripts";
    if (!std::filesystem::exists(scriptsDir) || !std::filesystem::is_directory(scriptsDir)) {
        std::cerr << "Scripts directory not found: " << scriptsDir << "\n";
        return 1;
    }

    std::vector<std::filesystem::path> scripts;
    for (auto &entry : std::filesystem::recursive_directory_iterator(scriptsDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".ril") scripts.push_back(entry.path());
    }

    if (scripts.empty()) {
        std::cout << "No .ril scripts found in " << scriptsDir << "\n";
        return 0;
    }

    std::cout << "Found " << scripts.size() << " script(s)\n\n";
    std::cout << std::fixed << std::setprecision(3);

    struct Result { std::string path; double compileMs; bool compileSuccess; double runMs; std::string error; };
    std::vector<Result> results;

    for (auto &scriptPath : scripts) {
        auto vm = EMI::CreateEnvironment();
        std::string relPath = scriptPath.string();
        std::cout << "\n----------------------------------------\n";
        std::cout << "\nCompiling: " << relPath << " ... " << std::flush;

        Result res; res.path = relPath; res.compileMs = 0.0; res.compileSuccess = false; res.runMs = -1.0; res.error = "";

        auto t0 = high_resolution_clock::now();
        try {
            auto compileTask = vm.CompileScript(relPath.c_str());
            res.compileSuccess = compileTask.wait();
            if (!res.compileSuccess) {
				throw(std::exception("Compilation failed"));
            }
        }
        catch (const std::exception &e) {
            auto t1 = high_resolution_clock::now();
            res.error = e.what();
            res.compileSuccess = false;
            res.compileMs = duration_cast<duration<double, std::milli>>(t1 - t0).count();
            std::cout << "\nFailed after " << res.compileMs << " ms (exception: " << res.error << ")\n";
            results.push_back(res);
            continue;
        }
        catch (...) {
            auto t1 = high_resolution_clock::now();
            res.error = "unknown error";
            res.compileSuccess = false;
            res.compileMs = duration_cast<duration<double, std::milli>>(t1 - t0).count();
            std::cout << "\nFailed after " << res.compileMs << " ms (unknown error)\n";
            results.push_back(res);
            continue;
        }

        auto t1 = high_resolution_clock::now();
        res.compileMs = duration_cast<duration<double, std::milli>>(t1 - t0).count();
        std::cout << "\nDone in " << res.compileMs << " ms\n";

        // Attempt to run script. Try several common entrypoints
		bool ran = false;
        try {
            EMI::FunctionHandle fh = vm.GetFunctionHandle("run_benchmark");
            if (!fh) throw(std::exception("run_benchmark not found!"));
            std::cout << "Running entry..." << std::flush;
            auto r0 = high_resolution_clock::now();
            auto runTask = fh();
            try { runTask.get<int>(); } catch (...) {}
            auto r1 = high_resolution_clock::now();
            res.runMs = duration_cast<duration<double, std::milli>>(r1 - r0).count();
            std::cout << "\nDone in " << res.runMs << " ms\n";
            ran = true;
        }
        catch (const std::exception &e) {
            res.error += std::string("entry failed: ") + e.what() + "\n";
        }
        catch (...) {
            res.error += std::string("entry failed with unknown error\n");
        }

        if (!ran) {
            std::cout << "No runnable entrypoint found for " << relPath << " (skipping run)\n";
            if (res.error.empty()) res.error = "No runnable entrypoint found";
        }

        results.push_back(res);
        EMI::ReleaseEnvironment(vm);
    }

    // Write results to JSON
    std::filesystem::path outPath = "benchmark_results.json";
    std::ofstream ofs(outPath);
    if (!ofs) {
        std::cerr << "Failed to open output file: " << outPath << "\n";
    } else {
        ofs << "[\n";
        for (size_t i = 0; i < results.size(); ++i) {
            const auto &r = results[i];
            ofs << "  {\n";
            ofs << "    \"script\": \"" << json_escape(r.path) << "\",\n";
            ofs << "    \"compile_ms\": " << r.compileMs << ",\n";
            ofs << "    \"compile_success\": " << (r.compileSuccess ? "true" : "false") << ",\n";
            if (r.runMs >= 0.0)
                ofs << "    \"run_ms\": " << r.runMs << ",\n";
            else
                ofs << "    \"run_ms\": null,\n";
            ofs << "    \"error\": ";
            if (!r.error.empty()) ofs << "\"" << json_escape(r.error) << "\"\n";
            else ofs << "null\n";
            ofs << "  }" << (i + 1 < results.size() ? "," : "") << "\n";
        }
        ofs << "]\n";
        ofs.close();
        std::cout << "\nResults written to: " << outPath << "\n";
    }

    return 0;
}