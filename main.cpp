//
//  main.cpp
//  CommentEliminator
//
//  Created by homeland on 17/6/24.
//  Copyright © 2017年 weilai. All rights reserved.
//

//standard headers
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

//standard template
#include <vector>
#include <map>
#include <algorithm>
#include <functional>

//system related headers
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include "pthread.h"

using namespace std;

class FileRecord {
public:
    ///Full path of the source file
    string path;
    ///The size of the source file in bytes
    int size;

    FileRecord(string path,int size) {
        this->path = path;
        this->size = size;
    }

    //The compare is based on the size
    //To Support sort
    bool operator <(const FileRecord& Right) const {
        return size < Right.size;
    }

    bool operator >(const FileRecord& Right) const {
        return size > Right.size;
    }

};

//The List of Source File, include path and size of file
vector<FileRecord> FileList;

//Input: the Path of File which need to be processed
//Output:
//Side effect: The File will be altered by removing all comments
void removeComment(string FilePath) {
    //FILE handle of the input file, read-only
    FILE * Input = fopen(FilePath.c_str(),"r");
    //If fail to open the file, show the message and leave funtion
    if (Input == NULL) {
        cout << "Can not open this file, path:"  << FilePath << "\n";
        return ;
    }
    //This Buffer is used to store the content of entire output file
    //Initialized as empty string
    string Buffer = "";
    //Previous is the last character readed
    //Now is the current character from input file
    char Previous = 0,Now = 0,PoP = 0;
    //0 for code, 1 for string, 2 for one line comment, 3 for multiline comment
    //The
    int Status = 0;
    int es = 0;
    //keep read the input stream until End of File
    //one character each time
    while (fscanf(Input, "%c",&Now) != EOF) {
        switch (Status) {
            //if cursor is in the normal code area
            case 0:{
                if (Previous == '/') {
                    if (Now == '/') {
                        Status = 2;
                    } else if (Now == '*') {
                        Status = 3;
                    } else {
                        Buffer += Previous;
                        Buffer += Now;
                    }
                } else if (Now != '/') {
                    Buffer += Now;
                    if (Previous != '\\' && Previous != '\'' && Now == '"') {
                        Status = 1;
                    }
                }
            }break;
            //if cursor is in a string
            case 1:{
                //if there is a single \ to continue the line, just put it into Buffer,
                //together with the \r or \n in Now
                if (Previous == '\\') {
                    if (es != 1) {
                        Buffer += Previous;
                        Buffer += Now;
                        es = 0;
                    } else {
                        Buffer += Now;
                        if (Now == '"') {
                            Status = 0;
                        }
                        es = 0;
                    }
                    if (Now == '\\') {
                        es = 1;
                    }
                //otherwise just add Now to Buffer
                } else if (Now != '\\') {
                    Buffer += Now;
                    if (Now == '"') {
                        Status = 0;

                    }
                }
            }break;
            //if cursor is in a single line comment
            case 2:{
                //if there is no single \ to continue the line, then stop single lien comment, add /n into Buffer
                if (Previous != '\\' && (Now == '\n' || Now == '\r')) {
                    Buffer += Now;
                    Now = 0;
                    Status = 0;
                }
            }break;
            //if cursor is in a multiple line comment
            case 3:{
                if (Previous == '*' && Now == '/') {
                    Status = 0;
                    Now = 0;
                }
            }break;
            default: {

            }break;
        }
        //update the Previous character, move forward to next character
        PoP = Previous;
        Previous = Now;
    }
    //close the input file
    fclose(Input);
    //open the input file in write mode
    ofstream fout(FilePath.c_str());
    //write the Buffer content into the file
    fout << Buffer;
    //close the file
    fout.close();
}


//Input, the File name with extension name
//Output, true if it's a source file, otherwise return false
bool isSource(string Name) {
    //The last 2 suffix
    string Bisuf = Name.c_str() + (Name.length() - 2);
    //The last 3 suffix
    string Trisuf = Name.c_str() + (Name.length() - 3);
    //The last 4 suffix
    string Fousuf = Name.c_str() + (Name.length() - 4);

    if (Bisuf == ".c" || Bisuf == ".h") {
        return true;
    }
    if (Trisuf == ".cc" || Trisuf ==".hh") {
        return true;
    }
    if (Fousuf == ".hpp" || Fousuf == ".cpp") {
        return true;
    }
    return false;
}


//Input: the directory path
//Output: 0 if finished all source file in the directory are added in FileList
//Functionality: recursively add all source file in FileList
//within the directory
int initializeSourceFileDir(string Dir) {
    if (DIR* Dp = opendir(Dir.c_str())) {
        //if can not open the Directory, display the error
        if (Dp == NULL) {
            cout << strerror(errno);
        }
        //read every item in the directory
        while (struct dirent * Ep = readdir(Dp)) {
            //if it's a regular file
            if (Ep->d_type == DT_REG) {
                // and it's a source file
                if (isSource(Ep->d_name)) {
                    //construct the full path of the source file
                    string FilePath = Dir + "/" + Ep->d_name;
                    cout << "Adding source file " << FilePath << endl;
                    int TempFileSize = 0;
                    struct stat StatBuffer;
                    if (stat(FilePath.c_str(),&StatBuffer) < 0) {
                        //just use 0
                    } else {
                        TempFileSize = StatBuffer.st_size;
                    }

                    FileList.push_back(FileRecord(FilePath,TempFileSize));
                    cout << "Source file added" << endl;
                }
            //if it's a directory
            } else if (Ep->d_type == DT_DIR) {
                //if it's not a (current directory or father directory)
                if (strncmp(Ep->d_name,".",100000) != 0 && strncmp(Ep->d_name,"..",100000) != 0){
                    cout << "Entering Directory " << Dir << endl;
                    //recursive call to the child directory
                    initializeSourceFileDir(Dir + "/" + Ep->d_name);
                    cout << "Leaving Directory " << endl;
                }
            }
        }
        //close the directory handle
        closedir(Dp);
    }
    return 0;
}


void displayFileList() {
    for (vector<FileRecord>::iterator It = FileList.begin();It != FileList.end();It++) {
        cout << It->path << " " << It->size << endl;
    }
}


void* threadEliminator(void* FilesPointer) {
    vector<string> *Files = (vector<string> *)FilesPointer;
    if (Files->size() <= 0) {
        return NULL;
    }
    for (vector<string>::iterator It = Files->begin(); It != Files->end(); It++) {
        string FilePath = *It;
        cout << "Removing comment from file " << FilePath << endl;
        removeComment(FilePath);
        cout << "Comment removed" << endl;
    }
}

//Distribute the Source File tasks to Threads
//Using Randomized strategy
void balanceThreads(int NumThread) {
    vector<string> Files[NumThread];
    std::vector<pthread_t> ThreadPointers(NumThread);

    for (int I = 0;I < NumThread; I++) {
        Files[I].clear();
    }

    for (int I = 0;I < FileList.size();I++) {
        int TargetThread = rand() % NumThread;
        Files[TargetThread].push_back(FileList[I].path);
    }

    for (int I = 0;I < NumThread;I++) {
        pthread_create(&ThreadPointers[I],NULL,threadEliminator,&(Files[I]));
    }
    for (int I = 0; I < NumThread; I++) {
        pthread_join(ThreadPointers[I], NULL);
    }
}

int sum(vector<FileRecord> FileList) {
    int Result = 0;
    for (vector<FileRecord>::iterator It = FileList.begin();It != FileList.end(); It++) {
        Result += It->size;
    }
    return Result;
}


bool completeDistribute(bool* used,int size) {
    int Result = true;
    for (int I = 0;I < size;I++) {
        if (!used[I]) {
            return false;
        }
    }
    return Result;
}
//Distribute the Source File tasks to Threads
//Using Greedy strategy
void balanceThreadsGreedy(int NumThread) {
    vector<string> Files[NumThread];
    std::vector<pthread_t> ThreadPointers(NumThread);

    //sort the Source File to descent order on size
    sort(FileList.begin(),FileList.end(),greater<FileRecord>());
    //displayFileList();

    //expected load for each thread
    double Average = sum(FileList) / (double)NumThread;

    //label of whether the File[i] is dispatched already
    bool used[FileList.size()];
    for (int I = 0;I < FileList.size();I++) {
        used[I] = false;
    }

    for (int I = 0;I < NumThread; I++) {
        Files[I].clear();
    }

    //gready distribute the task
    for (int I = 0; I < NumThread; I++) {
        int Capacity = Average;
        for (int J = 0;J < FileList.size();J++) {
            if (!used[J] && FileList[J].size <= Capacity) {
                Files[I].push_back(FileList[J].path);
                Capacity -= FileList[J].size;
                used[J] = true;
                break;
            }
        }
    }

    //Those task can not be handled are assigned randomly
    for (int I = 0; I < FileList.size();I++) {
        if (!used[I]) {
            int TargetThread = rand() % NumThread;
            Files[TargetThread].push_back(FileList[I].path);
            used[I] = true;
        }
    }

    //check whether all task all assigned
    if (completeDistribute(used,FileList.size())) {
        cout << "All tasks are assigned." << endl;
    } else {
        cout << "There are some mistake during the task assignment." << endl;
    }

    for (int I = 0;I < NumThread;I++) {
        pthread_create(&ThreadPointers[I],NULL,threadEliminator,&(Files[I]));
    }
    for (int I = 0; I < NumThread; I++) {
        pthread_join(ThreadPointers[I], NULL);
    }
}


void showHelp() {
    cout << "Use -a to choose algorithm 0:greedy 1:random\n"
            "Use -t to set the number of threads you want to use\n"
            "Use Directory of Project as Argument"
         << endl;
}


int main(int argc, char * argv[]) {
    srand(time(NULL));

    //The Directory of target Project
    string ProjectDir;
    //The number of threads,default = 1
    int NumThread = 1;
    int Strategy = 0;

    int Option;

    while ((Option = getopt(argc,argv,"ht:a:")) != -1) {
        switch (Option) {
        case 'h':
            showHelp();
            break;
        case 't':
            NumThread = atoi(optarg);
            break;
        case 'a':
            Strategy = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-h] [-t threads] [-a method] targetDirectory\n",argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }
    
    ProjectDir = argv[optind];

    //initialize the FileList to include all source file
    //in project directory and its size
    initializeSourceFileDir(ProjectDir);
    //sort the FileList based on size
    //sort(FileList.begin(),FileList.end(),less<FileRecord>());
    //display the FileList
    //displayFileList();
    //distribute the task to these threads

    if (NumThread <= 0) {
        fprintf(stderr, "Number of Thread must be bigger than 0\n");
        exit(EXIT_FAILURE);
    }

    switch (Strategy) {
    case 0:{
        balanceThreadsGreedy(NumThread);
    }
        break;
    case 1:{
        balanceThreads(NumThread);
    }
        break;
    default: {
        fprintf(stderr, "Invalid Algorithm parameter\n");
        exit(EXIT_FAILURE);
    }
        break;
    };
    
    return 0;
}
