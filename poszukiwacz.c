#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

short int check[65536] = {0}; 
int counterRepeated = 0;

typedef struct __attribute__((packed)) record {
    unsigned short int number;
    pid_t pid;
} record; 

void stdinCheck();
int argCheckUnit(char* arg);
void writeRecord(struct record* toWrite);
int ceiling(float num);
void readData();

int main(int argc, char* argv[]){
    if(argc == 2){
        stdinCheck();

        int number = argCheckUnit(argv[1]);

        for(int i = 0; i < number; i++) readData();

        int factor = ceiling(((float)counterRepeated/(float)number)*10);

        exit(factor);
    }
    else{
        fprintf(stderr, "Error: wrong number of arguments passed to \"poszukiwacz\"!\n");
        exit(16);
    }
}

int argCheckUnit(char* arg){
    char* end;
    int number = strtol(arg, &end, 0);
    
    if(*end != '\0' && *end != 'K' && *end != 'M' ) exit(11);
    else if(number <= 0) exit(11);
    else if(*end == 'K' && *(end+1) == 'i') number *= 1024;
    else if(*end == 'M' && *(end+1) == 'i') number *= 1024*1024;

    return number;
}

void readData(){
    short unsigned int buff;
    int readRes = read(0, &buff, 2);
    if( readRes != 2){
        //perror("ERROR reading a record/EOF") - no need to print
        exit(15);
    }
    
    if(check[buff] == 0){
        struct record toWrite = {buff, getpid()};
        writeRecord(&toWrite);
        check[buff] = '1';
    }
    else counterRepeated++;
}

void stdinCheck(){
    struct stat resStat;
    int resFstat = fstat(0, &resStat);
    
    if(resFstat == -1){
        perror("ERROR checking type of stdin descriptor: ");
        exit(12);
    }

    if(S_ISFIFO(resStat.st_mode) != 0) 
        return;

    fprintf(stderr, "Wrong type of stdin descriptor\n");
    exit(13);
}

void writeRecord(struct record *toWrite){
    if( write(1, toWrite, sizeof(record)) != sizeof(record)){
        perror("ERROR writing a record");
        exit(15);
    }
}

int ceiling(float num) {
    int inum = (int)num;
    if (num == (float)inum) {
        return inum;
    }
    return inum + 1;
}