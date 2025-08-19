// combine_sources.cpp
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <set>
#include <string>

// C++23: We'd use std::print, but it's not available yet
// Instead, we use std::cout with UTF-8 assumptions

namespace fs = std::filesystem;

// Supported source file extensions
const std::set<std::string> sourceExtensions = {
    ".cpp", ".cc", ".cxx", ".c",
    ".hpp", ".hh", ".hxx", ".h",
    ".hs", ".dart", ".js", ".ts", 
    ".ml", ".kt"
};

bool hasSourceExtension(const fs::path& path) {
    return sourceExtensions.contains(path.extension().string());
}

std::string readFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "[Error: Could not read file]\n";

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    // Ensure file ends with newline
    if (!content.empty() && content.back() != '\n') {
        content += '\n';
    }
    return content;
}

int main(int argc, char* argv[]) {

    fs::path inputDir = ".";      // default: current directory
    fs::path outputFile = "combined_sources.txt";
    std::ofstream outFile(outputFile);

    if (argc > 1) inputDir = argv[1];
    if (argc > 2) outputFile = argv[2];

    if (!fs::exists(inputDir)) { std::cerr << "Error: Directory '" << inputDir << "' does not exist.\n"; return 1; }
    if (!fs::is_directory(inputDir)) { std::cerr << "Error: '" << inputDir << "' is not a directory.\n"; return 1; }
    if (!outFile) { std::cerr << "Error: Could not create output file '" << outputFile << "'.\n"; return 1; }

    std::vector<fs::path> sourceFiles;

    // C++17+ range-based loop over directory entries (C++23 style usage)
    for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {

        if(entry.is_regular_file() && hasSourceExtension( entry.path() )) {

            sourceFiles.push_back( entry.path() );
        }
    }

    // Sort for consistent output
    //std::ranges::sort(sourceFiles);
    std::sort(sourceFiles.begin(), sourceFiles.end());

    // Write each file with header
    for (const auto& filePath : sourceFiles) {

        outFile << "-- file name: " << filePath << " --\n";
        std::string content = readFile( filePath );
        outFile << content;
    }

    std::cout << "✅ Combined " << sourceFiles.size() << " source files into '"
              << outputFile << "'\n";

    return 0;
}
