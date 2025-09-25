// combine_by_subdir.cpp
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <functional>

namespace fs = std::filesystem;

// ANSI color codes for ayu-mirage theme
const std::string RESET = "\033[0m";
const std::string TREE_COLOR = "\033[38;5;243m";    // muted gray for tree characters
const std::string DIR_COLOR = "\033[38;5;109m";     // blue for directories
const std::string FILE_COLOR = "\033[38;5;248m";    // light gray for files
const std::string COUNT_COLOR = "\033[38;5;142m";   // green for line counts
const std::string HEADER_COLOR = "\033[38;5;180m";  // orange for headers


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

// Function to build and display directory structure tree
void displayDirectoryTree(const fs::path& baseDir, const fs::path& subdir) {
    std::cout << "\n" << HEADER_COLOR << "📁 Directory Structure for: " << subdir.filename().string() << RESET << "\n";
    std::cout << "```\n";

    // Use a simpler approach: collect all directories and files in order
    std::map<std::string, std::vector<std::pair<std::string, int>>> dirContents;

    // First pass: collect all source files with their line counts
    for (const auto& fileEntry : fs::recursive_directory_iterator(subdir)) {
        if (fileEntry.is_regular_file() && hasSourceExtension(fileEntry.path())) {
            fs::path relativePath = fs::relative(fileEntry.path(), baseDir);
            std::string parentPath = relativePath.parent_path().string();
            if (parentPath.empty()) parentPath = ".";

            std::string filename = relativePath.filename().string();
            auto [content, lineCount] = readFileWithLineCount(fileEntry.path());

            dirContents[parentPath].push_back({filename, lineCount});
        }
    }

    // Build directory tree structure
    std::set<std::string> allDirs;
    for (const auto& [parentPath, files] : dirContents) {
        allDirs.insert(parentPath);

        // Add parent directories
        std::string currentPath = parentPath;
        while (!currentPath.empty() && currentPath != ".") {
            allDirs.insert(currentPath);
            size_t lastSlash = currentPath.find_last_of('/');
            if (lastSlash == std::string::npos) break;
            currentPath = currentPath.substr(0, lastSlash);
        }
    }

    // Create a map of directory to its subdirectories
    std::map<std::string, std::vector<std::string>> subdirs;
    for (const auto& dirPath : allDirs) {
        // Find all subdirectories of this directory
        for (const auto& subdirPath : allDirs) {
            if (subdirPath != dirPath && subdirPath.find(dirPath + "/") == 0) {
                std::string subdirName = subdirPath.substr(dirPath.length() + 1);
                size_t nextSlash = subdirName.find('/');
                if (nextSlash != std::string::npos) {
                    subdirName = subdirName.substr(0, nextSlash);
                }
                if (std::find(subdirs[dirPath].begin(), subdirs[dirPath].end(), subdirName) == subdirs[dirPath].end()) {
                    subdirs[dirPath].push_back(subdirName);
                }
            }
        }
    }

    // Display the tree structure
    std::function<void(const std::string&, const std::string&, bool)> printTree =
        [&](const std::string& path, const std::string& prefix, bool isLast) {
            // Print current directory
            size_t lastSlash = path.find_last_of('/');
            std::string displayName = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

            std::cout << prefix;
            std::cout << TREE_COLOR << (isLast ? "└── " : "├── ") << RESET;
            std::cout << DIR_COLOR << displayName << RESET << "\n";

            // Get subdirectories and files for this path
            std::vector<std::string> children;

            // Add subdirectories
            if (subdirs.find(path) != subdirs.end()) {
                for (const auto& subdir : subdirs[path]) {
                    children.push_back(subdir);
                }
            }

            // Add files
            if (dirContents.find(path) != dirContents.end()) {
                for (const auto& [filename, lineCount] : dirContents[path]) {
                    children.push_back(filename + " (" + std::to_string(lineCount) + " lines)");
                }
            }

            // Print children
            std::string newPrefix = prefix + (isLast ? "    " : "│   ");
            for (size_t i = 0; i < children.size(); ++i) {
                const auto& child = children[i];
                // Check if this is a file (contains line count) or directory
                if (child.find("lines)") != std::string::npos) {
                    // This is a file with line count
                    size_t parenPos = child.find(" (");
                    if (parenPos != std::string::npos) {
                        std::string filename = child.substr(0, parenPos);
                        std::string lineInfo = child.substr(parenPos);
                        std::cout << newPrefix;
                        std::cout << TREE_COLOR << (i == children.size() - 1 ? "└── " : "├── ") << RESET;
                        std::cout << FILE_COLOR << filename << RESET;
                        std::cout << COUNT_COLOR << lineInfo << RESET << "\n";
                    }
                } else {
                    // This is a directory
                    printTree((path == "." ? "" : path + "/") + child, newPrefix, i == children.size() - 1);
                }
            }
        };

    // Find root directories (those without '/' in their path)
    std::vector<std::string> rootDirs;
    for (const auto& dirPath : allDirs) {
        if (dirPath.find('/') == std::string::npos) {
            rootDirs.push_back(dirPath);
        }
    }

    // Sort root directories for consistent output
    std::sort(rootDirs.begin(), rootDirs.end());

    // Print each root directory
    for (size_t i = 0; i < rootDirs.size(); ++i) {
        bool isLast = (i == rootDirs.size() - 1);
        printTree(rootDirs[i], "", isLast);
    }

    std::cout << "```\n\n";
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

        // Display directory structure with line counts
        displayDirectoryTree(baseDir, subdir);

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
