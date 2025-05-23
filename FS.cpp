#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <ctime>

namespace fs = std::filesystem;

class Logger {
private:
    std::ofstream logFile;
public:
    // 构造函数，创建日志文件
    Logger(){
        std::string Time = getTimeString();
        std::filesystem::path logPath = "logs/log_" + Time + ".txt";
        fs::create_directories(logPath.parent_path()); 
        logFile.open(logPath, std::ios::app);
        if (logFile.is_open()) {
            logFile << "Log started at " << Time << std::endl;
        } else {
            std::cerr << "Logger error: failed to open log file" << std::endl;
        }
    }
    // 日志记录，并封装输出
    void log(const std::string& message, std::string level) {
        if (logFile.is_open()) {
            logFile <<"["<< getTimeString() <<"]"<< message << std::endl;
        } else {
            std::cerr << "Logger error: file is not open" << std::endl;
        }
        if (level == "error") {
            std::cerr << message << std::endl;
        } else if (level == "warning") {
            std::cout << "Warning: " << message << std::endl;
        } else if (level == "info") {
            std::cout << message << std::endl;
        }
    }
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    // 用于获取时间字符串
    std::string getTimeString() const {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H:%M:%S", ltm);
        return std::string(timeStr);
    }
};


class FileCreator {
public:
    FileCreator() = default;
    ~FileCreator() = default;
    // touch
    std::string createFile(const std::string& fileName) const {
        try {
            std::ofstream file(fileName);
            if (file.is_open()) {
                file.close();
                return "File created: " + fileName;
            } else {
                return "Error creating file: " + fileName;
            }
        } catch (const std::exception& e) {
            return "Error: " + std::string(e.what());
        }
    }
};


class DirectoryBase {
protected:
    std::string dirPath;
public:
    DirectoryBase(const std::string& path) : dirPath(path) {}
    virtual ~DirectoryBase() {}
    virtual std::string getCurrentDir() const {
        return dirPath;
    }
};

class DirectoryOperator : virtual public DirectoryBase {
public:
    DirectoryOperator(const std::string& path) : DirectoryBase(path) {}
    ~DirectoryOperator() override = default;
    // mkdir
    std::string createDirectory(const std::string& dirName) const {
        try {
            fs::create_directory(dirName);
            return ("Directory created: " + dirName);
        } catch (const fs::filesystem_error& e) {
            return ("Error creating directory: " + std::string(e.what()));
        }
    }
    // cd
    void changeDirectory(const std::string& targetDir) { 
        try {
            if (targetDir == "..") {
                fs::path current = fs::path(this->dirPath);
                if (current == fs::path("/")) {
                    std::cerr << "Already at the root directory." << std::endl;
                    return;
                }
                if (current.has_parent_path()) {
                    fs::current_path(current.parent_path());
                    this->dirPath = fs::current_path().string();
                }
            } else {
                fs::path targetPath = targetDir;
                if (targetDir[0] != '/') {  
                    targetPath = fs::path(this->dirPath) / targetDir;
                }
                
                if (fs::is_directory(targetPath)) {
                    fs::current_path(targetPath);
                    this->dirPath = fs::current_path().string();
                } else {
                    std::cerr << "Invalid directory: " << targetDir << std::endl;
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error changing directory: " << e.what() << std::endl;
        }
    }
};

class FileSystemOperator : public FileCreator, public DirectoryOperator {
public:
    FileSystemOperator() 
        : FileCreator(), 
          DirectoryBase(fs::current_path().string()), 
          DirectoryOperator(fs::current_path().string()){}

    ~FileSystemOperator() override = default;
    // ls
    std::string listDirectory(const std::string& dirPath = ".") const {
        try {
            std::string result = "\n";
            std::string dirs = "";
            std::string files = "";
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (entry.is_directory()) {
                    dirs += "    Dir: " + std::string(entry.path().filename()) + "\n";
                } else if (entry.is_regular_file()) {
                    files += "    File: " + std::string(entry.path().filename()) + "\n";
                }
            }
            result += dirs + files;
            return result;
        } catch (const fs::filesystem_error& e) {
            return "Error listing directory: " + std::string(e.what());
        }
    }
    // rm
    std::string removeItem(const std::string& itemName) const {
        try {
            if (fs::is_directory(itemName)) {
                fs::remove_all(itemName);
                return "Directory removed: " + itemName;
            } else if (fs::is_regular_file(itemName)) {
                fs::remove(itemName);
                return "File removed: " + itemName;
            } else {
                return "Invalid item: " + itemName;
            }
        } catch (const fs::filesystem_error& e) {
            return "Error removing item: " + std::string(e.what());
        }
    }
};


std::vector<std::string> splitCommand(const std::string& command) {
    std::vector<std::string> args;
    size_t start = 0, end;
    while ((end = command.find(' ', start)) != std::string::npos) {
        if (end > start) {
            args.push_back(command.substr(start, end - start));
        }
        start = end + 1;
    }
    if (start < command.size()) {
        args.push_back(command.substr(start));
    }
    return args;
}
void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void printHelp() {
    std::cout << "\n    FileSystem commands:" << std::endl;
    std::cout << "        mkdir <directory_name> - create a new directory" << std::endl;
    std::cout << "        touch <file_name> - create a new file" << std::endl;
    std::cout << "        ls [directory_path] - list files and directories in the current or specified directory" << std::endl;
    std::cout << "        rm <file_or_directory_name> - remove a file or directory" << std::endl;
    std::cout << "        cd <directory_path> - change the current directory" << std::endl;
    std::cout << "        clear - clear the screen" << std::endl;
    std::cout << "        exit - exit the program\n" << std::endl;
}

int main() {
    Logger logger;
    FileSystemOperator fsOp;
    std::string command;
    while (true) {
        std::cout << "FileSystem: " << fsOp.getCurrentDir() << "> ";
        if (!std::getline(std::cin, command)){
            break;
        }

        if (command == "exit") {
            logger.log("Exiting program", "");
            break;
        }

        logger.log("Command: " + command, "");

        std::vector<std::string> args = splitCommand(command);
        if (args.empty()) {
            continue;
        }

        if (args[0] == "mkdir") {
            if (args.size() == 2) {
                logger.log(fsOp.createDirectory(args[1]), "info");
            } else {
                logger.log("Usage: mkdir <directory_name>", "error");
            }
        } else if (args[0] == "touch") {
            if (args.size() == 2) {
                logger.log(fsOp.createFile(args[1]), "info");
            } else {
                logger.log("Usage: touch <file_name>", "error");
            }
        } else if (args[0] == "ls") {
            if (args.size() == 1) {
                logger.log(fsOp.listDirectory(), "info");
            } else if (args.size() == 2) {
                logger.log(fsOp.listDirectory(args[1]), "info");
            } else {
                logger.log("Usage: ls [directory_path]", "error");
            }
        } else if (args[0] == "rm") {
            if (args.size() == 2) {
                logger.log(fsOp.removeItem(args[1]), "info");
            } else {
                logger.log("Usage: rm <file_or_directory_name>", "error");
            }
        } else if (args[0] == "cd") {
            if (args.size() == 2) {
                fsOp.changeDirectory(args[1]);
            } else {
                logger.log("Usage: cd <directory_path>", "error");
            }
        } else if (args[0] == "clear") {
            clearScreen();
        } else if (args[0] == "help") {
            printHelp();
        } else {
            logger.log("Unknown command: " + args[0], "error");
        }
    }
    return 0;
}