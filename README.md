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
You can run this manually or use our script `./run_build.sh` to configure everything. The command has three modes
* without flags it generates the `build` and `build_coverage` directories and calls cmake on them
* with `--all` the same as the previous one but also runs `make` and `make coverage` respectively
* with `--clean` deletes the `build` and `build_coverage` directories

From here you can manually `cd` into which ever build you want to test and run a `make` command to test.
