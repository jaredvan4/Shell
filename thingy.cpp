#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;
// I found this on a website
string subStrFileName(string fileName){
    string subStr = fileName;
    subStr = subStr.substr(subStr.find(" .") + 3);
    return subStr;
}
string getCommandFromFile(string fileName){
    string subStr = subStrFileName(fileName);
    ifstream file(subStr);
    string command;
    if (file) {
        ostringstream strStream;
        strStream << file.rdbuf();
        command = strStream.str();
    }
    return command;
}
vector<string> parsePath(string path) {
    vector<string> paths;
    stringstream stream(path);
    while (stream.good()) {
        string pathPortion;
        getline(stream, pathPortion, ':');
        paths.push_back(pathPortion);
    }
    return paths;
}
void pathExec(const char *input,char **args){
    int exec = execv(args[0], args);
    if (exec == -1) { 
        cout << args[0] << "\n";
        perror("failed to exec ");
    }
}

void normalExec(vector<string>paths, char **args){
     for (int i = 0; i < paths.size(); i++) {
        string appendedPath = paths.at(i) + "/" + args[0];
        int exec = execv(appendedPath.c_str(), args);
    }
}
void fileExec(const char *input, char **args,vector<string>paths){
    string fileName(input);
    string commandFromFile = getCommandFromFile(fileName);
    int scan = sscanf(commandFromFile.c_str(), "%ms %ms %ms %ms %ms %ms", &args[0], &args[1], &args[2], &args[3], &args[4], &args[5]);
    if (strstr(args[0], "/") != nullptr){
        pathExec(input, args);
    }
    normalExec(paths,args);
}
void handleExec(const char *input,vector<string>paths){
    char *args[6];
    int scan = sscanf(input, "%ms %ms %ms %ms %ms %ms", &args[0], &args[1], &args[2], &args[3], &args[4], &args[5]);
    //if full executable path
    if (strstr(args[0], "/") != nullptr){
        pathExec(input, args);
    //if cmd is in file
    } else if (strstr(input, ". ") != nullptr){
        fileExec(input,args,paths);
    }
    //if normal
    normalExec(paths,args);
}

void printHistory() {
    HIST_ENTRY **list = history_list();
    HISTORY_STATE *histInfo = history_get_history_state();
    for (int i = 0; i < histInfo->length; i++) {
        cout << list[i]->line << "\n";
    }
}
void changeDir(char *line) {
    char *subStr = strstr(line, " ") + 1;
    int cd = chdir(subStr);
    if (cd == -1) {
        perror("Failed to cd\n");
    }
}
void handleInput(char *input) {
    add_history(input);
    if (strcmp(input, "history") == 0) {
        printHistory();
    }
    if (strcmp(input, "exit") == 0) {
        cout << "exiting...\n";
        exit(0);
    }
    if (strstr(input, "cd") != nullptr) {
        changeDir(input);
    } else {
        string path(getenv("PATH"));
        vector<string> paths = parsePath(path);
        int status;
        int pid = fork();
        if (pid == 0) {
            handleExec(input,paths);
            cout << "error bad!!!\n";
            exit(1);
        } else if (pid > 0) {
            int something = 0;
            do {
                int something = waitpid(-1, &status, WNOHANG);
            } while (something > 0);
        }
    }
}

int main(int argc, char *argv[]) {
    while (true) {
        char *input = readline("$ ");
        handleInput(input);
    }
}
