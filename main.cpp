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
#include <cstring>

namespace fs = std::filesystem;

#ifdef _WIN32
#include <windows.h>
bool enableVirtualTerminal() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return false;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return false;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    return SetConsoleMode(hOut, dwMode);
}
#endif

bool useColors = true;

void initColors() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    useColors = enableVirtualTerminal();
#endif
}

std::string RESET = "";
std::string TREE_COLOR = "";
std::string DIR_COLOR = "";
std::string FILE_COLOR = "";
std::string COUNT_COLOR = "";
std::string HEADER_COLOR = "";

void setColors() {
    if (!useColors) return;
    RESET = "\033[0m";
    TREE_COLOR = "\033[38;5;243m";
    DIR_COLOR = "\033[38;5;109m";
    FILE_COLOR = "\033[38;5;248m";
    COUNT_COLOR = "\033[38;5;142m";
    HEADER_COLOR = "\033[38;5;180m";
}

std::string convertEmojis(const std::string& text) {
#ifdef _WIN32
    std::string result = text;
    size_t pos = std::string::npos;
    while ((pos = result.find("📦")) != std::string::npos) {
        result.replace(pos, 3, "[P]");
    }
    while ((pos = result.find("📁")) != std::string::npos) {
        result.replace(pos, 3, "[DIR]");
    }
    while ((pos = result.find("✅")) != std::string::npos) {
        result.replace(pos, 3, "[OK]");
    }
    while ((pos = result.find("🎉")) != std::string::npos) {
        result.replace(pos, 3, "[DONE]");
    }
    return result;
#else
    return text;
#endif
}

char getPathSeparator() {
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

std::string normalizePathSep(const std::string& path) {
    std::string result = path;
    for (char& c : result) {
        if (c == '\\') c = '/';
    }
    return result;
}


const std::unordered_map<std::string, std::string> extToLang = {
    {".cpp", "cpp"}, {".cc", "cpp"}, {".cxx", "cpp"},
    {".c",   "c"},
    {".hpp", "h"},   {".hh", "h"},   {".hxx", "h"}, {".h", "h"},
    {".m",   "objc"},
    {".rs", "rust"},
    {".go", "go"},
    {".java", "java"},
    {".cs", "csharp"},
    {".php", "php"},
    {".zig", "zig"},
    {".odin", "odin"},
    {".nim", "nim"},
    {".swift", "swift"},
    {".py",  "python"},
    {".js",  "javascript"},
    {".tsx", "typescript"},
    {".ts",  "typescript"},
    {".purs", "purescript"},
    {".scala", "scala"},
    {".dart", "dart"},
    {".ml", "ocaml"},
    {".hs", "haskell"},
    {".css", "css"},
    {".md", "markdown"},
    {".json", "json"},
    {".txt", "text"}
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

bool isExcluded(const fs::path& dirPath, const fs::path& baseDir, const std::set<std::string>& excludedDirs) {
    if (excludedDirs.empty()) return false;
    
    // Check direct filename (e.g., "node_modules")
    std::string dirName = dirPath.filename().string();
    if (excludedDirs.count(dirName) > 0) {
        return true;
    }

    // Check relative path (e.g., "src/ignored")
    try {
        fs::path relPath = fs::relative(dirPath, baseDir);
        std::string relPathStr = normalizePathSep(relPath.string());
        if (excludedDirs.count(relPathStr) > 0) {
            return true;
        }
    } catch (...) {
        // Ignore errors
    }

    return false;
}

// Returns a pair of (content, line_count)
std::pair<std::string, int> readFileWithLineCount(const fs::path& path) {
    std::ifstream file(path);
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
void displayDirectoryTree(const fs::path& baseDir, const fs::path& subdir, const std::set<std::string>& excludedDirs) {
    std::cout << convertEmojis("\n" + HEADER_COLOR + "📁 Directory Structure for: " + subdir.filename().string() + RESET + "\n");
    std::cout << "```\n";

    // Use a simpler approach: collect all directories and files in order
    std::map<std::string, std::vector<std::pair<std::string, int>>> dirContents;

    // First pass: collect all source files with their line counts
    for (auto it = fs::recursive_directory_iterator(subdir); it != fs::recursive_directory_iterator(); ++it) {
        if (it->is_directory()) {
            if (isExcluded(it->path(), baseDir, excludedDirs)) {
                it.disable_recursion_pending();
            }
        } else if (it->is_regular_file() && hasSourceExtension(it->path())) {
            fs::path relativePath = fs::relative(it->path(), baseDir);
            std::string parentPath = normalizePathSep(relativePath.parent_path().string());
            if (parentPath.empty()) parentPath = ".";

            std::string filename = relativePath.filename().string();
            auto [content, lineCount] = readFileWithLineCount(it->path());

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
    initColors();
    setColors();

    fs::path baseDir = "."; // default to current directory
    std::set<std::string> excludedDirs;
    bool baseDirSet = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--exclude") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --exclude requires a list of directories.\n";
                std::cerr << "Usage: " << argv[0] << " [--exclude dir1,dir2,...] [directory]\n";
                return 1;
            }
            std::string excludesStr = argv[++i];
            // Split by comma
            size_t start = 0;
            size_t end = excludesStr.find(',');
            while (end != std::string::npos) {
                std::string token = excludesStr.substr(start, end - start);
                if (!token.empty()) {
                    excludedDirs.insert(normalizePathSep(token));
                }
                start = end + 1;
                end = excludesStr.find(',', start);
            }
            std::string lastToken = excludesStr.substr(start);
            if (!lastToken.empty()) {
                excludedDirs.insert(normalizePathSep(lastToken));
            }
        } else if (arg.rfind("-", 0) == 0) { // Any option starting with - (e.g. -h, --help)
            std::cerr << "Error: Unknown option '" << arg << "'\n";
            std::cerr << "Usage: " << argv[0] << " [--exclude dir1,dir2,...] [directory]\n";
            return 1;
        } else {
            if (baseDirSet) {
                std::cerr << "Error: Multiple directories specified.\n";
                std::cerr << "Usage: " << argv[0] << " [--exclude dir1,dir2,...] [directory]\n";
                return 1;
            }
            baseDir = arg;
            baseDirSet = true;
        }
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

        // Skip excluded directories
        if (isExcluded(subdir, baseDir, excludedDirs)) {
            continue;
        }

        std::cout << convertEmojis("📦 Processing subdirectory: " + subdir.filename().string() + "\n");

        // Display directory structure with line counts
        displayDirectoryTree(baseDir, subdir, excludedDirs);

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
        for (auto it = fs::recursive_directory_iterator(subdir); it != fs::recursive_directory_iterator(); ++it) {
            if (it->is_directory()) {
                if (isExcluded(it->path(), baseDir, excludedDirs)) {
                    it.disable_recursion_pending();
                }
            } else if (it->is_regular_file() && hasSourceExtension(it->path())) {
                sourceFiles.push_back(it->path());
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
            outFile << "```\n\n";

            subdirLines += lineCount;
        }

        outFile.close();
        std::cout << convertEmojis("✅ Wrote " + std::to_string(sourceFiles.size()) + " files to '" + outputFileName.string() + "' (" + std::to_string(subdirLines) + " lines)\n");
        processedSubdirs++;
        totalLines += subdirLines;
    }

    std::cout << convertEmojis("🎉 Done. Processed " + std::to_string(processedSubdirs) + " subdirectories. " + std::to_string(totalLines) + " lines.\n");
    return 0;
}
