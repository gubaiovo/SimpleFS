#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <ctime>

namespace fs = std::filesystem;


class Logger {
private:
    std::ofstream logfile;
public:
    Logger(){
        std::string Time = getTimeString();
        fs::path logpath = fs::path(fs::current_path()) / "log" / (Time + ".log");
        fs::create_directories(logpath.parent_path()); 
        logfile.open(logpath, std::ios::app);
        if (logfile.is_open()) {
            logfile << "Log started at " << Time << std::endl;
        } else {
            std::cerr << "Logger error: failed to open log file" << std::endl;
            std::cerr << "Log file path: " << logpath << std::endl;
        }
    }
    void log(const std::string& message, std::string level) {
        if (logfile.is_open()) {
            logfile <<"["<< getTimeString() <<"]"<< message << std::endl;
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
        if (logfile.is_open()) {
            logfile.close();
        }
    }
    std::string getTimeString() const {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H-%M-%S", ltm);
        return std::string(timeStr);
    }
};

class Base {
protected:
    std::string path;
public:
    Base() : path("") {}
    Base(const std::string& path) : path(path) {}
    virtual ~Base() {}
    virtual std::string getPath() const { return path; }
};

class FileOp: virtual public Base {
public:
    FileOp() = default;
    ~FileOp() override = default;
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
    // cat
    std::string readFile(const std::string& fileName) const {
        try {
            std::ifstream file(fileName);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                file.close();
                return content;
            } else {
                return "Error reading file: " + fileName;
            }
        } catch (const std::exception& e) {
            return "Error: " + std::string(e.what());
        }
    }

};

class DirOp : virtual public Base {
public:
    DirOp(const std::string& path) : Base(path) {}
    ~DirOp() override = default;

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
    void changeDir(const std::string& targetDir) { 
        try {
            if (targetDir == "..") {
                fs::path current = fs::path(this->path);
                if (current == fs::path("/")) {
                    std::cerr << "Already at the root directory." << std::endl;
                    return;
                }
                if (current.has_parent_path()) {
                    fs::current_path(current.parent_path());
                    this->path = fs::current_path().string();
                }
            } else {
                fs::path targetPath = targetDir;
                if (targetDir[0] != '/') {  
                    targetPath = fs::path(this->path) / targetDir;
                }
                
                if (fs::is_directory(targetPath)) {
                    fs::current_path(targetPath);
                    this->path = fs::current_path().string();
                } else {
                    std::cerr << "Invalid directory: " << targetDir << std::endl;
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error changing directory: " << e.what() << std::endl;
        }
    }
    
    // ls
    std::string listDir(const std::string& dirPath = ".") const {
        try {
            std::string result = "\n";
            std::string dirs = "";
            std::string files = "";
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (entry.is_directory()) {
                    dirs += "    Dir: " + entry.path().filename().string()  + "\n";
                } else if (entry.is_regular_file()) {
                    files += "    File: " + entry.path().filename().string()  + "\n";
                }
            }
            result += dirs + files;
            return result;
        } catch (const fs::filesystem_error& e) {
            return "Error listing directory: " + std::string(e.what());
        }
    }
};

class FSOp : public FileOp, public DirOp {
public:
    FSOp() 
        : Base(fs::current_path().string()), 
          FileOp(), 
          DirOp(fs::current_path().string()){}
    ~FSOp() override = default;

    // rm (file or directory)
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

    // mv (move file or directory to another directory)
    std::string moveFile(const std::string& src, const std::string& dest) const {
        try {
            fs::path srcPath = src;
            if (src[0] != '/') {  
                srcPath = fs::path(this->path) / src;
            }
            fs::path destPath = dest;
            if (dest[0] != '/') {  
                destPath = fs::path(this->path) / dest;
            }
            fs::rename(srcPath, destPath);
            return "File moved: " + src + " -> " + dest;
        } catch (const fs::filesystem_error& e) {
            return "Error moving file: " + std::string(e.what());
        }
    }

    // cp
    std::string copyFile(const std::string& src, const std::string& dest) const {
        try {
            fs::copy_file(src, dest);
            return "File copied: " + src + " -> " + dest;
        } catch (const fs::filesystem_error& e) {
            return "Error copying file: " + std::string(e.what());
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
    std::cout << "        cat <file_name> - display the contents of a file" << std::endl;
    std::cout << "        cp <source_file> <destination_file> - copy a file" << std::endl;
    std::cout << "        mv <source_file> <destination_file> - move a file" << std::endl;
    std::cout << "        pwd - display the current directory" << std::endl;
    std::cout << "        clear - clear the screen" << std::endl;
    std::cout << "        exit - exit the program\n" << std::endl;
}

int main() {
    Logger logger;
    FSOp FSOp;
    std::string command;
    while (true) {
        std::cout << "FileSystem: " << FSOp.getPath() << "> ";
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
                logger.log(FSOp.createDirectory(args[1]), "info");
            } else {
                logger.log("Usage: mkdir <directory_name>", "error");
            }
        } else if (args[0] == "touch") {
            if (args.size() == 2) {
                logger.log(FSOp.createFile(args[1]), "info");
            } else {
                logger.log("Usage: touch <file_name>", "error");
            }
        } else if (args[0] == "ls") {
            if (args.size() == 1) {
                logger.log(FSOp.listDir(), "info");
            } else if (args.size() == 2) {
                logger.log(FSOp.listDir(args[1]), "info");
            } else {
                logger.log("Usage: ls [directory_path]", "error");
            }
        } else if (args[0] == "rm") {
            if (args.size() == 2) {
                logger.log(FSOp.removeItem(args[1]), "info");
            } else {
                logger.log("Usage: rm <file_or_directory_name>", "error");
            }
        } else if (args[0] == "cd") {
            if (args.size() == 2) {
                FSOp.changeDir(args[1]);
            } else {
                logger.log("Usage: cd <directory_path>", "error");
            }
        } else if (args[0] == "cat") {
            if (args.size() == 2) {
                logger.log(FSOp.readFile(args[1]), "info");
            } else {
                logger.log("Usage: cat <file_name>", "error");
            }
        } else if (args[0] == "cp") {
            if (args.size() == 3) {
                logger.log(FSOp.copyFile(args[1], args[2]), "info");
            } else {
                logger.log("Usage: cp <source_file> <destination_file>", "error");
            }
        } else if (args[0] == "mv") {
            if (args.size() == 3) {
                logger.log(FSOp.moveFile(args[1], args[2]), "info");
            } else {
                logger.log("Usage: mv <source_file> <destination_file>", "error");
            }
        } else if (args[0] == "pwd") {
            logger.log(FSOp.getPath(), "info");
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