// combine_by_subdir.cpp
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <unordered_map>

namespace fs = std::filesystem;


const std::unordered_map<std::string, std::string> extToLang = {
    {".cpp", "cpp"}, {".cc", "cpp"}, {".cxx", "cpp"},
    {".c",   "c"},
    {".hpp", "h"},   {".hh", "h"},   {".hxx", "h"}, {".h", "h"},
    {".m",   "objc"},
    {".swift", "swift"},
    {".py",  "python"},
    {".js",  "javascript"},
    {".dart", "dart"},
    {".ml", "ocaml"},
    {".hs", "haskell"}
};


// Get language ID for syntax highlighting (e.g., "cpp", "c", "h")
std::string getLang(const fs::path& path) {
    std::string ext = path.extension().string();
    auto it = extToLang.find(ext);
    return (it != extToLang.end()) ? it->second : "text";
}

// Extract set of valid extensions from the map (for filtering)
std::set<std::string> getSourceExtensions() {
    std::set<std::string> extensions;
    for (const auto& [ext, lang] : extToLang) {
        extensions.insert(ext);
    }
    return extensions;
}

// Now we get the set from the map — DRY principle!
const std::set<std::string> sourceExtensions = getSourceExtensions();


bool hasSourceExtension(const fs::path& path) {
    return sourceExtensions.find(path.extension().string()) != sourceExtensions.end();
}

// Returns a pair of (content, line_count)
std::pair<std::string, int> readFileWithLineCount(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {"[Error: Could not read file]\n", 0};

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    // Count lines in the content
    int lineCount = 0;
    if (!content.empty()) {
        lineCount = 1; // At least one line
        for (char c : content) {
            if (c == '\n') {
                lineCount++;
            }
        }
    }
    
    // Ensure ends with newline
    if (!content.empty() && content.back() != '\n') {
        content += '\n';
    }
    return {content, lineCount};
}

int main(int argc, char* argv[]) {
    fs::path baseDir = "."; // default to current directory
    
    // Parse command line arguments
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [directory]\n";
        std::cerr << "If no directory is specified, uses current directory.\n";
        return 1;
    }
    
    if (argc == 2) {
        baseDir = argv[1];
    }

    if (!fs::exists(baseDir) || !fs::is_directory(baseDir)) {
        std::cerr << "Error: Cannot access directory '" << baseDir << "'\n";
        return 1;
    }

    int processedSubdirs = 0;
    int totalLines = 0;

    // Iterate only over immediate subdirectories (depth 1)
    for (const auto& entry : fs::directory_iterator(baseDir)) {
        if (!entry.is_directory()) continue;

        const fs::path& subdir = entry.path();
        std::cout << "📦 Processing subdirectory: " << subdir.filename() << "\n";

        // Create combine-txt directory if it doesn't exist
        fs::path outputDir = "combine-txt";
        if (!fs::exists(outputDir)) {
            fs::create_directory(outputDir);
        }

        // Output file: combine-txt/subdir_name.txt
        fs::path outputFileName = outputDir / (subdir.filename().string() + ".txt");
        std::ofstream outFile(outputFileName);
        if (!outFile) {
            std::cerr << "Error: Could not create output file '" << outputFileName << "'\n";
            continue;
        }

        std::vector<fs::path> sourceFiles;
        int subdirLines = 0;

        // Recursively scan all files in this subdirectory (and nested folders)
        for (const auto& fileEntry : fs::recursive_directory_iterator(subdir)) {
            if (fileEntry.is_regular_file() && hasSourceExtension(fileEntry.path())) {
                sourceFiles.push_back(fileEntry.path());
            }
        }

        // Sort for consistent output
        std::sort(sourceFiles.begin(), sourceFiles.end());

        // Write each file with header and code block
        for (const auto& filePath : sourceFiles) {
            std::string relativePath = fs::relative(filePath, baseDir).string();
            std::string lang = getLang(filePath);
            auto [content, lineCount] = readFileWithLineCount(filePath);

            outFile << "--- file: " << relativePath << " ---\n";
            outFile << "```" << lang << "\n";
            outFile << content;
            outFile << "```\n\n"; // closing + empty line
            
            subdirLines += lineCount;
        }

        outFile.close();
        std::cout << "✅ Wrote " << sourceFiles.size() << " files to '" << outputFileName << "' (" << subdirLines << " lines)\n";
        processedSubdirs++;
        totalLines += subdirLines;
    }

    std::cout << "🎉 Done. Processed " << processedSubdirs << " subdirectories. " << totalLines << " lines.\n";
    return 0;
}
