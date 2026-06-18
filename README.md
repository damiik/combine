# combine

A C++ tool that scans subdirectories, combines source files into formatted text files, and displays directory structure with line counts.

## Supported Languages

cpp, c, h, objc, rust, go, java, csharp, php, zig, odin, nim, swift, python, javascript, typescript, purescript, scala, dart, ocaml, haskell, css, markdown, json, text

## Build

```bash
g++ -std=c++17 -O2 -o combine main.cpp
```

## Usage

```bash
./combine [--exclude dir1,dir2,...] [directory]
```

If no directory is specified, uses the current directory.

- `--exclude`: Specifies a list of directory names or relative paths to exclude from processing and scanning. Multiple directories can be separated by commas (e.g., `--exclude node_modules,build`) or by specifying multiple `--exclude` options.

## Output

- Creates `combine-txt/` directory
- For each subdirectory, outputs a `.txt` file with all source files wrapped in fenced code blocks with language identifiers
- Displays ASCII directory tree with line counts per file

## Example

```
./combine myproject
```

Processes each immediate subdirectory of `myproject/`, combining its source files into `combine-txt/subdir_name.txt`.

## Testing

A Python test suite is available to verify the directory exclusion and combination logic. To run the tests:

```bash
python run_tests.py
```