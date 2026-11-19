#include "main.h"

struct Error {
	const std::string message;
	const std::string filePath;
	const std::string function;
	const std::string source;
	const std::string line;
};

static bool isCommandLine;
static bool isProgressBarActive = false;
static uint32_t filesSkipped = 0;

static struct {
	bool showHelp = false;
	bool silentAssertions = false;
	bool forceOverwrite = false;
	bool ignoreDebugInfo = false;
	bool minimizeDiffs = false;
	bool unrestrictedAscii = false;
	std::string inputPath;
	std::string outputPath;
	std::string extensionFilter;
} arguments;

struct Directory {
	const std::string path;
	std::vector<Directory> folders;
	std::vector<std::string> files;
};

static std::string string_to_lowercase(const std::string& string) {
	std::string lowercaseString = string;

	for (uint32_t i = lowercaseString.size(); i--;) {
		if (lowercaseString[i] < 'A' || lowercaseString[i] > 'Z') continue;
		lowercaseString[i] += 'a' - 'A';
	}

	return lowercaseString;
}

static std::string get_file_extension(const std::string& filename) {
	const size_t dotPos = filename.rfind('.');
	if (dotPos == std::string::npos || dotPos == filename.size() - 1) return "";
	return filename.substr(dotPos);
}

static void find_files_recursively(Directory& directory) {
	std::string dirPath = arguments.inputPath + directory.path;
	if (dirPath.empty()) dirPath = "/";
	DIR* handle = opendir(dirPath.c_str());
	if (handle == nullptr) return;

	struct dirent* entry;

	while ((entry = readdir(handle)) != nullptr) {
		if (!std::strcmp(entry->d_name, ".") || !std::strcmp(entry->d_name, "..")) continue;
		std::string fullPath = dirPath + entry->d_name;
		struct stat st;

		if (stat(fullPath.c_str(), &st) != 0) continue;

		if (S_ISDIR(st.st_mode)) {
			directory.folders.emplace_back(Directory{ .path = directory.path + entry->d_name + "/" });
			find_files_recursively(directory.folders.back());
			if (!directory.folders.back().files.size() && !directory.folders.back().folders.size()) directory.folders.pop_back();
			continue;
		}

		if (!arguments.extensionFilter.size() || arguments.extensionFilter == string_to_lowercase(get_file_extension(entry->d_name))) directory.files.emplace_back(entry->d_name);
	}

	closedir(handle);
}

static bool decompile_files_recursively(const Directory& directory) {
	const std::string dirPath = arguments.outputPath + directory.path;
	if (dirPath != "") {
		std::string parent = dirPath;
		while (parent.size() > 1 && parent.back() == '/') {
			parent.pop_back();
			if (mkdir(parent.c_str(), 0755) == 0 || errno == EEXIST) break;
		}
	}
	std::string outputFile;

	for (uint32_t i = 0; i < directory.files.size(); i++) {
		outputFile = directory.files[i];
		const size_t dotPos = outputFile.rfind('.');
		if (dotPos != std::string::npos && dotPos > 0) outputFile.erase(dotPos);
		outputFile += ".lua";

		Bytecode bytecode(arguments.inputPath + directory.path + directory.files[i]);
		Ast ast(bytecode, arguments.ignoreDebugInfo, arguments.minimizeDiffs);
		Lua lua(bytecode, ast, arguments.outputPath + directory.path + outputFile, arguments.forceOverwrite, arguments.minimizeDiffs, arguments.unrestrictedAscii);

		try {
			print("--------------------\nInput file: " + bytecode.filePath + "\nReading bytecode...");
			bytecode();
			print("Building ast...");
			ast();
			print("Writing lua source...");
			lua();
			print("Output file: " + lua.filePath);
		} catch (const Error& error) {
			erase_progress_bar();

			if (arguments.silentAssertions) {
				print("\nError running " + error.function + "\nSource: " + error.source + ":" + error.line + "\n\n" + error.message);
				filesSkipped++;
				continue;
			}

			print("\nError running " + error.function + "\nSource: " + error.source + ":" + error.line + "\n\nFile: " + error.filePath + "\n\n" + error.message + "\nFile skipped.");
			filesSkipped++;
		} catch (...) {
			print("\nUnknown exception\n\nFile: " + bytecode.filePath);
			throw;
		}
	}

	for (uint32_t i = 0; i < directory.folders.size(); i++) {
		if (!decompile_files_recursively(directory.folders[i])) return false;
	}

	return true;
}

static char* parse_arguments(const int& argc, char** const& argv) {
	if (argc < 2) return nullptr;
	arguments.inputPath = argv[1];
#ifndef _DEBUG
	if (!isCommandLine) return nullptr;
#endif
	bool isInputPathSet = true;

	if (arguments.inputPath.size() && arguments.inputPath.front() == '-') {
		arguments.inputPath.clear();
		isInputPathSet = false;
	}

	std::string argument;

	for (uint32_t i = isInputPathSet ? 2 : 1; i < argc; i++) {
		argument = argv[i];

		if (argument.size() >= 2 && argument.front() == '-') {
			if (argument[1] == '-') {
				argument = argument.c_str() + 2;

				if (argument == "extension") {
					if (i <= argc - 2) {
						i++;
						arguments.extensionFilter = argv[i];
						continue;
					}
				} else if (argument == "force_overwrite") {
					arguments.forceOverwrite = true;
					continue;
				} else if (argument == "help") {
					arguments.showHelp = true;
					continue;
				} else if (argument == "ignore_debug_info") {
					arguments.ignoreDebugInfo = true;
					continue;
				} else if (argument == "minimize_diffs") {
					arguments.minimizeDiffs = true;
					continue;
				} else if (argument == "output") {
					if (i <= argc - 2) {
						i++;
						arguments.outputPath = argv[i];
						continue;
					}
				} else if (argument == "silent_assertions") {
					arguments.silentAssertions = true;
					continue;
				} else if (argument == "unrestricted_ascii") {
					arguments.unrestrictedAscii = true;
					continue;
				}
			} else if (argument.size() == 2) {
				switch (argument[1]) {
				case 'e':
					if (i > argc - 2) break;
					i++;
					arguments.extensionFilter = argv[i];
					continue;
				case 'f':
					arguments.forceOverwrite = true;
					continue;
				case '?':
				case 'h':
					arguments.showHelp = true;
					continue;
				case 'i':
					arguments.ignoreDebugInfo = true;
					continue;
				case 'm':
					arguments.minimizeDiffs = true;
					continue;
				case 'o':
					if (i > argc - 2) break;
					i++;
					arguments.outputPath = argv[i];
					continue;
				case 's':
					arguments.silentAssertions = true;
					continue;
				case 'u':
					arguments.unrestrictedAscii = true;
					continue;
				}
			}
		}

		return argv[i];
	}

	return nullptr;
}

static void wait_for_exit() {
	// no-op on Linux; console apps don't need this
}

int main(int argc, char* argv[]) {
	isCommandLine = true;

	print(std::string(PROGRAM_NAME) + "\nCompiled on " + __DATE__);
	
	if (parse_arguments(argc, argv)) {
		print("Invalid argument: " + std::string(parse_arguments(argc, argv)) + "\nUse -? to show usage and options.");
		return EXIT_FAILURE;
	}
	
	if (arguments.showHelp) {
		print(
			"Usage: luajit-decompiler-v2.exe INPUT_PATH [options]\n"
			"\n"
			"Available options:\n"
			"  -h, -?, --help\t\tShow this message\n"
			"  -o, --output OUTPUT_PATH\tOverride default output directory\n"
			"  -e, --extension EXTENSION\tOnly decompile files with the specified extension\n"
			"  -s, --silent_assertions\tDisable assertion error pop-up window\n"
			"\t\t\t\t  and auto skip files that fail to decompile\n"
			"  -f, --force_overwrite\t\tAlways overwrite existing files\n"
			"  -i, --ignore_debug_info\tIgnore bytecode debug info\n"
			"  -m, --minimize_diffs\t\tOptimize output formatting to help minimize diffs\n"
			"  -u, --unrestricted_ascii\tDisable default UTF-8 encoding and string restrictions"
		);
		return EXIT_SUCCESS;
	}
	
	if (!arguments.inputPath.size()) {
		print("No input path specified!");
		return EXIT_FAILURE;
	}

	if (!arguments.outputPath.size()) {
		char exePath[4096];
		if (readlink("/proc/self/exe", exePath, sizeof(exePath) - 1) != -1) {
			exePath[sizeof(exePath) - 1] = '\0';
			std::string path(exePath);
			const size_t lastSlash = path.rfind('/');
			arguments.outputPath = (lastSlash != std::string::npos ? path.substr(0, lastSlash) : ".") + "/output/";
		} else {
			arguments.outputPath = "./output/";
		}
		arguments.outputPath.shrink_to_fit();
	} else {
		struct stat outputPath;

		if (stat(arguments.outputPath.c_str(), &outputPath) != 0) {
			print("Failed to open output path: " + arguments.outputPath);
			return EXIT_FAILURE;
		}

		if (!S_ISDIR(outputPath.st_mode)) {
			print("Output path is not a folder!");
			return EXIT_FAILURE;
		}

		switch (arguments.outputPath.back()) {
		case '/':
			break;
		default:
			arguments.outputPath += '/';
			break;
		}
	}

	if (arguments.extensionFilter.size()) {
		if (arguments.extensionFilter.front() != '.') arguments.extensionFilter.insert(arguments.extensionFilter.begin(), '.');
		arguments.extensionFilter = string_to_lowercase(arguments.extensionFilter);
	}

	struct stat inputPath;

	if (stat(arguments.inputPath.c_str(), &inputPath) != 0) {
		print("Failed to open input path: " + arguments.inputPath);
		wait_for_exit();
		return EXIT_FAILURE;
	}

	Directory root;

	if (S_ISDIR(inputPath.st_mode)) {
		switch (arguments.inputPath.back()) {
		case '/':
			break;
		default:
			arguments.inputPath += '/';
			break;
		}

		find_files_recursively(root);

		if (!root.files.size() && !root.folders.size()) {
			print("No files " + (arguments.extensionFilter.size() ? "with extension " + arguments.extensionFilter + " " : "") + "found in path: " + arguments.inputPath);
			wait_for_exit();
			return EXIT_FAILURE;
		}
	} else {
		const size_t lastSlash = arguments.inputPath.rfind('/');
		root.files.emplace_back(lastSlash != std::string::npos ? arguments.inputPath.substr(lastSlash + 1) : arguments.inputPath);
		arguments.inputPath = lastSlash != std::string::npos ? arguments.inputPath.substr(0, lastSlash + 1) : "";
	}

	try {
		if (!decompile_files_recursively(root)) {
			print("--------------------\nAborted!");
			wait_for_exit();
			return EXIT_FAILURE;
		}
	} catch (...) {
		throw;
	}

#ifndef _DEBUG
	print("--------------------\n" + (filesSkipped ? "Failed to decompile " + std::to_string(filesSkipped) + " file" + (filesSkipped > 1 ? "s" : "") + ".\n" : "") + "Done!");
	wait_for_exit();
#endif
	return EXIT_SUCCESS;
}

void print(const std::string& message) {
	std::printf("%s\n", message.c_str());
}

void print_progress_bar(const double& progress, const double& total) {
	static char PROGRESS_BAR[] = "\r[====================]";

	const uint8_t threshold = std::round(20 / total * progress);

	for (uint8_t i = 20; i--;) {
		PROGRESS_BAR[i + 2] = i < threshold ? '=' : ' ';
	}

	std::fputs(PROGRESS_BAR, stdout);
	std::fflush(stdout);
	isProgressBarActive = true;
}

void erase_progress_bar() {
	static constexpr char PROGRESS_BAR_ERASER[] = "\r                      \r";

	if (!isProgressBarActive) return;
	std::fputs(PROGRESS_BAR_ERASER, stdout);
	std::fflush(stdout);
	isProgressBarActive = false;
}

void assert(const bool& assertion, const std::string& message, const std::string& filePath, const std::string& function, const std::string& source, const uint32_t& line) {
	if (!assertion) throw Error{
		.message = message,
		.filePath = filePath,
		.function = function,
		.source = source,
		.line = std::to_string(line)
	};
}

std::string byte_to_string(const uint8_t& byte) {
	char string[] = "0x00";
	uint8_t digit;
	
	for (uint8_t i = 2; i--;) {
		digit = (byte >> i * 4) & 0xF;
		string[3 - i] = digit >= 0xA ? 'A' + digit - 0xA : '0' + digit;
	}

	return string;
}
