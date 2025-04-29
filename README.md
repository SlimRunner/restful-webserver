# HTTP Server
## C++ Development
### Setup
To keep code clean do the following to setup auto-formatting:

First, add the following to your `.vscode/settings.json`
```jsonc
{
  "[cpp]": {
    "editor.tabSize": 4,
    "editor.insertSpaces": true
  },
  "C_Cpp.clang_format_style": "file", // Use .clang-format if available
  "C_Cpp.clang_format_fallbackStyle": "Google", // Or LLVM, Mozilla, etc.
  "editor.formatOnSave": true
}
```
Append, this to whatever is in there already. This file is not tracked because it contains noise that the C++ extension adds. With this in there now the `.clang-format` will setup all the rules we want for formatting. Lastly, this makes it so that the format is applied every time you save to make it easier to keep a consistent code style.

### Devel Environment
This project uses a docker container to run the project. To set that up run
```sh
 ../tools/env/start.sh -u mariomar314 -r
```
Assuming that you have your devel tools one directory up.

### Run CMake
You can run this manually or use our script `./run_build.sh` to configure everything. The command has the following modes:

* `--clean`: Deletes the `build` and `build_coverage` directories.
* `--build`: Builds the release version only.
* `--int`: Updates the build with CMake and runs integration tests only.
* `--cover`: Builds the coverage version only.
* `-h`, `--help`: get this same help in the terminal

You can still manually `cd` into the desired build directory and run the appropriate `make` commands to test or execute the build.
