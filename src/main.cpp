#include "Models.hpp"
#include "IOUtils.hpp"
#include "Aberrations.hpp"
#include "MathUtils.hpp"

#include "json.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <fstream>

using json = nlohmann::json;

static void printUsage() {
    std::cout << "Usage:\n"
              << "  ./run                          Use config.json\n"
              << "  ./run --config <path>          Use custom config\n"
              << "  ./run compute --input <f.json> [--output <f.csv>] [--print]\n"
              << "  ./run compute76 --infinity <i.json> --finite <f.json> [--output <f.csv>]\n";
}

static void printResults(const std::vector<optical::ResultItem>& results) {
    for (const auto& r : results) {
        std::cout << std::setw(2) << std::right << r.orderIndex << " "
                  << "[" << r.caseName << "] "
                  << std::setw(12) << std::left << r.category << " "
                  << r.name << " = ";
        if (std::isnan(r.value)) {
            std::cout << "NaN";
        } else if (std::isinf(r.value)) {
            std::cout << (r.value > 0 ? "Inf" : "-Inf");
        } else {
            std::cout.precision(6);
            std::cout << std::fixed << r.value;
        }
        std::cout << " " << r.unit << "\n";
    }
}

int main(int argc, char* argv[]) {
    // --- If no explicit command given, read config.json ---
    bool hasCommandArg = (argc >= 2 && argv[1][0] != '-' && std::strcmp(argv[1], "--config") != 0);

    if (!hasCommandArg) {
        std::string configPath = "config.json";
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
                configPath = argv[++i];
            }
        }

        std::ifstream f(configPath);
        if (f.is_open()) {
            try {
                json cfg;
                f >> cfg;

                std::string mode = cfg.value("mode", "compute76");
                std::vector<std::string> fakeArgs = {"optical_raytrace", mode};

                if (mode == "compute") {
                    std::string inp = cfg.value("input", "examples/test_infinity.json");
                    std::string out = cfg.value("output", "");
                    bool prt = cfg.value("print", false);
                    fakeArgs.push_back("--input");
                    fakeArgs.push_back(inp);
                    if (!out.empty()) { fakeArgs.push_back("--output"); fakeArgs.push_back(out); }
                    if (prt) fakeArgs.push_back("--print");
                } else if (mode == "compute76") {
                    std::string inf = cfg.value("infinity", "examples/test_infinity.json");
                    std::string fin = cfg.value("finite", "examples/test_finite.json");
                    std::string out = cfg.value("output", "outputs/result_76.csv");
                    fakeArgs.push_back("--infinity"); fakeArgs.push_back(inf);
                    fakeArgs.push_back("--finite"); fakeArgs.push_back(fin);
                    if (!out.empty()) { fakeArgs.push_back("--output"); fakeArgs.push_back(out); }
                } else {
                    std::cerr << "Unknown mode in config: " << mode << "\n";
                    return 1;
                }

                std::vector<char*> newArgv;
                for (auto& s : fakeArgs) newArgv.push_back(&s[0]);
                return main(static_cast<int>(newArgv.size()), newArgv.data());
            } catch (const std::exception& e) {
                std::cerr << "Config error: " << e.what() << "\n";
                return 1;
            }
        }
    }

    // --- Original CLI logic below (unchanged) ---
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "compute") {
        std::string inputPath, outputPath;
        bool printFlag = false;

        for (int i = 2; i < argc; ++i) {
            if (std::strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
                inputPath = argv[++i];
            } else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
                outputPath = argv[++i];
            } else if (std::strcmp(argv[i], "--print") == 0) {
                printFlag = true;
            }
        }

        if (inputPath.empty()) {
            std::cerr << "Error: --input required\n";
            return 1;
        }

        try {
            auto system = optical::loadSystemFromJson(inputPath);
            auto results = optical::compute38Values(system, "infinity");

            if (printFlag) printResults(results);
            if (!outputPath.empty()) {
                optical::saveResultsToCsv(results, outputPath);
                std::cout << "Results written to " << outputPath << "\n";
            }
            if (outputPath.empty() && !printFlag)
                std::cout << "Computed " << results.size() << " values.\n";
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }

    } else if (command == "compute76") {
        std::string infinityPath, finitePath, outputPath;

        for (int i = 2; i < argc; ++i) {
            if (std::strcmp(argv[i], "--infinity") == 0 && i + 1 < argc) {
                infinityPath = argv[++i];
            } else if (std::strcmp(argv[i], "--finite") == 0 && i + 1 < argc) {
                finitePath = argv[++i];
            } else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
                outputPath = argv[++i];
            }
        }

        if (infinityPath.empty() || finitePath.empty()) {
            std::cerr << "Error: --infinity and --finite required\n";
            return 1;
        }

        try {
            auto infSys = optical::loadSystemFromJson(infinityPath);
            auto finSys = optical::loadSystemFromJson(finitePath);
            auto results = optical::compute76Values(infSys, finSys);

            if (!outputPath.empty()) {
                optical::saveResultsToCsv(results, outputPath);
                std::cout << "76 results written to " << outputPath << "\n";
            } else {
                printResults(results);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }

    } else {
        std::cerr << "Unknown command: " << command << "\n";
        printUsage();
        return 1;
    }

    return 0;
}
