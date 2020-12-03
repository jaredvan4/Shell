#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;

int histOff;
vector<string> loadBlackList();
vector<string> commandQueue;
vector<string> blackList;
void handleExec(const char *input, vector<string> paths);
int findHistOff();
void handleInput(char *input,vector<string> paths);
void handleQueue(char *input,vector<string> paths);
void setEnvVar(char *input);
void changeDir(const char *line);
bool findInBlackList(string execName);

string subStrFileName(string fileName) {
    string subStr = fileName;
    subStr = subStr.substr(subStr.find_first_of(".") + 2);
    return subStr;
}

vector<string> getCommandsFromFile(string fileName) {
    vector<string> commandList;
    string subStr = subStrFileName(fileName);
    ifstream file;
        file.open(subStr);
        string command;
        if (file.is_open()) {
            while (getline(file, command)) {
                commandList.push_back(command);
            }
        } else { 
            cerr << "Error opening file: " << strerror(errno) << ":" << subStr << endl;
        }
    file.close();
    return commandList;
}

vector<string> loadBlackList() {
    vector<string> blackListNames;
    ifstream file("blacklist.txt");
    string line;
    if (file.is_open()) {
        while (getline(file, line)) {
            blackListNames.push_back(line);
        }
        file.close();
        return blackListNames;
    }
}

vector<string> parseStream(string path, char delimiter) {
    vector<string> collection;
    stringstream stream(path);
    while (stream.good()) {
        string portion;
        getline(stream, portion, delimiter);
        collection.push_back(portion);
    }
    return collection;
}

void pathExec(const char *input, char **args) {
    int exec = execv(args[0], args);
    if (exec == -1) {
        perror("command not found");
        cout << args[0] << "\n";
    }
}

string getCommandFromHistory(char **args) {
    string index(args[0]);
    index = index.substr(index.find("!") + 1);
    int indexInt = stoi(index);
    HIST_ENTRY **list = history_list();
    string command = list[indexInt - 1]->line;
    return command;
}

void bangExec(vector<string> paths, char **args) {
    string command = getCommandFromHistory(args);
    handleExec(command.c_str(), paths);
}

void normalExec(vector<string> paths, char **args) {
    for (int i = 0; i < paths.size(); i++) {
        string appendedPath = paths.at(i) + "/" + args[0];
        int exec = execv(appendedPath.c_str(), args);
    }
    cout << "command not found:" << args[0] << "\n";
}

void fileExec(vector<string> fileCommands, vector<string> paths) {
    for (int i = 0; i < fileCommands.size(); i++) {
        char *args[6];
        string command = fileCommands.at(i);
        int scan = sscanf(command.c_str(), "%ms %ms %ms %ms %ms %ms", &args[0],
                          &args[1], &args[2], &args[3], &args[4], &args[5]);
        if (findInBlackList(string(args[0]))) {
            cout << "Tried to run blacklisted executable:" << args[0] << "\n";
            return;
        }
        if (strcmp(command.c_str(), "exit") == 0) {
            cout << "exiting\n";
            int writeOffSet = findHistOff() - histOff;
            int addHist = append_history(writeOffSet,
                                         "/home/jared/dev/CS426/Shell/log.txt");
            if (addHist != 0) {
                perror("failed to write to history file");
            }
            cout << "exiting...\n";
            exit(0);
        } else if (strstr(command.c_str(), "=") != nullptr) {
            char envVarStr[command.length() + 1];
            strcpy(envVarStr, command.c_str());
            setEnvVar(envVarStr);
            // I had to add a fork for a command in a file, cause I was forking
            // when cd'in from a file command, which was no good
        } else if (strstr(args[0], "cd") != nullptr) {
            changeDir(command.c_str());
            // else fork for running processes from file
        } else {
            int status;
            int pid = fork();
            if (pid == 0) {
                if (strstr(args[0], "/") != nullptr) {
                    pathExec(command.c_str(), args);
                } else {
                    normalExec(paths, args);
                }
                exit(1);
            } else if (pid > 0) {
                int something = waitpid(-1, &status, 0);
            }
        }
    }
}

bool findInBlackList(string execName) {
    for (int i = 0; i < blackList.size(); i++) {
        if (blackList.at(i).find(execName) != string::npos) {
            return true;
        }
    }
    return false;
}

void handleExec(const char *input, vector<string> paths) {
    char *args[6];
    int scan = sscanf(input, "%ms %ms %ms %ms %ms %ms", &args[0], &args[1],
                      &args[2], &args[3], &args[4], &args[5]);
    if (findInBlackList(string(args[0]))) {
        cout << "Tried to run blacklisted executable:" << args[0] << "\n";;
        return;
    }
    // if full executable path
    if (strstr(args[0], "/") != nullptr) {
        pathExec(input, args);
    } else if (strstr(args[0], "!") != nullptr) {
        // if from history
        bangExec(paths, args);
    } else {
        // normal exec
        normalExec(paths, args);
    }
}

void printHistory() {
    HIST_ENTRY **list = history_list();
    HISTORY_STATE *histInfo = history_get_history_state();
    for (int i = 0; i < histInfo->length; i++) {
        cout << i + 1 << " " << list[i]->line << "\n";
    }
}

void changeDir(const char *line) {
    string subStr(line);
    subStr = subStr.substr(subStr.find("cd") + 3);
    int cd = chdir(subStr.c_str());
    if (cd == -1) {
        perror("Failed to cd\n");
        cout << "substr:" << subStr << "\n";
    }
}
string parseName(char *str) {
    string inputString(str);
    string name = inputString.substr(0, inputString.find('='));
    return name;
}
string parseValue(char *str) {
    string inputString(str);
    string value = inputString.substr(inputString.find('=') + 1);
    return value;
}
void setEnvVar(char *input) {
    string name = parseName(input);
    string value = parseValue(input);
    int env = setenv(name.c_str(), value.c_str(), 1);
    if (env == -1) {
        perror("failed to set env var:");
    }
}
void handleQueue(char *input, vector<string> paths) {
    commandQueue = parseStream(input, ';');
    for (int i = 0; i < commandQueue.size(); i++) {
        handleInput((char *)commandQueue.at(i).c_str(),paths);
    }
}

void handleInput(char *input, vector <string> paths) {
    if (input[0] != '!') {
        add_history(input);
    }
    if (strcmp(input, "history") == 0) {
        printHistory();
    } else if (strcmp(input, "exit") == 0) {
        int writeOffSet = findHistOff() - histOff;
        int addHist =
            append_history(writeOffSet, "/home/jared/dev/CS426/Shell/log.txt");
        if (addHist != 0) {
            perror("failed to write to history file");
        }
        cout << "exiting...\n";
        exit(0);
    } else if (strstr(input, "=") != nullptr) {
        setEnvVar(input);

    } else if (strstr(input, ";") != nullptr) {
        handleQueue(input,paths);
    } else if (strstr(input, "cd") != nullptr) {
        changeDir(input);
    } else if (strstr(input, ". ") != nullptr) {
        // command is in file
        fileExec(getCommandsFromFile(input), paths);
    } else {
        int status;
        int pid = fork();
        if (pid == 0) {
            handleExec(input, paths);
            exit(1);
        } else if (pid > 0) {
            int something = waitpid(-1, &status, 0);
        }
    }
}

int findHistOff() {
    HIST_ENTRY **list = history_list();
    HISTORY_STATE *histInfo = history_get_history_state();
    int off = histInfo->length;
    return off;
}
bool checkIfCstrIsWhiteSpace(char *string) {
    for (int i = 0; i < strlen(string) + 1; i++) {
        if (isspace(string[i]) != 0) {
        } else {
            return false;
        }
    }
    return true;
}
void handleSigint(int sig) {
    cout << endl << "$ ";
}

int main(int argc, char *argv[]) {
    vector<string> paths = parseStream(getenv("PATH"), ':');
    blackList = loadBlackList();
    read_history("log.txt");
    histOff = findHistOff();
    vector<string> commands = getCommandsFromFile(". .myshell");
    fileExec(commands, paths);
    while (true) {
        signal(SIGINT, handleSigint);
        char *input = readline("$ ");
        if (checkIfCstrIsWhiteSpace(input) == true || input[0] == '\0') {
            continue;
        }
        handleInput(input,paths);
    }
}
