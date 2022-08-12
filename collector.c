#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define N 49152
int childrenCounter = 0;
int counter = 0;
int isClosed = 0;
int readingPipe[2], writingPipe[2];

typedef struct __attribute__((packed)) successRecord{
    unsigned short int number;
    pid_t pid;
} successRecord; 

int argCheckUnit(char* arg);
int argCheck(char* arg);
int openFile(char* path, int method);
void fillFile(int fd);
void createPipe(int fds[2], int whichstd);
void child_do(char* blokString);
void parent_do(int mainFd, int achievementsFd, char* blokStr, int reportsFd, unsigned int* wolumen, unsigned int blok, unsigned int prac);
void redirectStd(int chanel, int fd);
void replaceProcess(char* blokString);
void parseArgLine(int argc, char* argv[], char* blokStr, int* mainFd, int* achievementsFd, int* reportsFd, unsigned int* wolumen,
                                                                                unsigned int* blok, unsigned int* prac);
int copyData(int fd, unsigned int* wolumen, char* buffer, int* readBytes); 
void writeRaportRecord(int fd, int reason, pid_t pid);        
int checkDeadChildren(char* blokString, int reportsFd);
void writeSuccessRecord(int fd, struct successRecord* record);        
int readSuccessRecord(int writeFd);
int globalFd;

int main(int argc, char* argv[]){
    createPipe(readingPipe, 0);
    createPipe(writingPipe, 1);

    int mainFd, achievementsFd, reportsFd = -1; 
    unsigned int wolumen, blok, prac = -1;
    char blokStr[10] = {'\0'};

    parseArgLine(argc, argv, blokStr, &mainFd, &achievementsFd, &reportsFd, &wolumen, &blok, &prac);

    int pid = 1;
    for(int i=0; i<prac; i++){
        if(pid == 0) break; 
        else if(pid == -1){
            perror("Error creating a child");
            exit(22);
        }
        pid = fork();
        if(pid > 0) {
            childrenCounter++;
            writeRaportRecord(reportsFd, 0, pid); 
        }
    }

    switch(pid){
        case -1:
            perror("ERROR caused by fork: ");
            exit(22);
        case 0:
            child_do(blokStr);
            break;
        default:
            parent_do(mainFd, achievementsFd, blokStr,reportsFd, &wolumen, blok, prac);
            break;
    }
}

void child_do(char* blokString){
    if(close(readingPipe[0]) == -1){
        perror("Error closing readingPipe[0]");
        exit(22);
    }

    if(isClosed == 0){
        if(close(writingPipe[1]) == -1){
            perror("Error closing writingPipe[1]");
            exit(22);
        }
    }

    redirectStd(0, writingPipe[0]);
    redirectStd(1, readingPipe[1]);

    replaceProcess(blokString);
}

void parent_do(int mainFd, int achievementsFd, char* blokStr, int reportsFd, unsigned int* wolumen, unsigned int blok, unsigned int prac){
    struct timespec time = {0, 480000000L};

    char buff[256] = {'\0'};
    int bytesRead; 

    while(1){
        copyData(mainFd, wolumen, buff, &bytesRead);
        int readRes = readSuccessRecord(achievementsFd);
        int checkRes = checkDeadChildren(blokStr, reportsFd);
        if(!readRes && !checkRes){
            if(nanosleep(&time, NULL) == -1){
                perror("Error nanosleep");
                exit(22);
            }
        }
    }
}

int copyData(int fd, unsigned int* wolumen, char* buffer, int* readBytes){
    if(*wolumen > 0){
        if(buffer[0] == '\0'){
            *readBytes = *wolumen < 256 ? *wolumen : 256;
            if(read(fd, buffer, *readBytes) != *readBytes){
                perror("ERROR using read (copyData)/EOF");
                exit(22);
            }
        }
 
        switch(write(writingPipe[1], buffer, *readBytes)){
            case -1: 
                if(errno == EAGAIN) 
                    return 0;
                else{
                    perror("ERROR writing data to pipe");
                    exit(22);
                }
            case 0:               
                fprintf(stderr, "EOF (write-copyData)\n");
                exit(22);
            default:
                *wolumen -= *readBytes;
                if(*wolumen == 0){
                    isClosed = 1; 
                    close(writingPipe[1]);
                }
                buffer[0] = '\0';
                return 1;
        }
    }
    return -1;
}                                                               

void redirectStd(int chanel, int fd){
    switch(chanel){
        case 0:
        case 1:
            if(dup2(fd, chanel) == -1){
                perror("Error redirecting stdin/stdout: ");
                exit(22);
            }
            break;
        default:
            fprintf(stderr, "Wrong channel passed to dup2\n");
            exit(13);
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

void replaceProcess(char* blokString){
    char* args[] = {"./poszukiwacz", blokString, NULL};
    if(execvp("./poszukiwacz", args) == -1){
        perror("Error using execvp: ");
        exit(22);
    }
}

int argCheck(char* arg){
    char* end;
    int number = strtol(arg, &end, 0);
    
    if(*end != '\0' || number <= 0){
	    fprintf(stderr, "Given <prac> is not a positive number\n");
	    exit(22);
    }
    return number;
}

int openFile(char* path, int method){
    int fd;
    switch(method){
        case 0:
            fd = open(path, O_RDONLY);
            break;
        case 1:
            fd = open(path, O_WRONLY | O_APPEND);
        case 2:
            fd = open(path, O_RDWR);
            break;
        default:
            fprintf(stderr, "Given method is not 0, 1 or 2\n");
	        exit(22);
    }
    if(fd == -1){
        perror("Error opening a file: ");
        exit(22);
    }
    return fd; 
}

void createPipe(int fds[2], int whichstd){
    if(pipe(fds) == -1){
        perror("Error creating a pipie: ");
        exit(22);
    }

    //https://www.linuxtoday.com/blog/blocking-and-non-blocking-i-0/
    int res;
    switch(whichstd){
        case 0:
            res = fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
            break;
        case 1: 
            res = fcntl(fds[1], F_SETFL, fcntl(fds[1], F_GETFL) | O_NONBLOCK);
            break;
        default:
            fprintf(stderr, "Error wrong arg passed (createPipe)!");
            exit(22);
    }

    if(res == -1){
        perror("Error chanhing to non-block mode");
        exit(22);
    }
}

void fillFile(int fd){
    pid_t buffer[65536] = {0};
    if(write(fd, buffer, 65536*sizeof(pid_t)) != 65536*sizeof(pid_t)){
        perror("Error filling the file");
        exit(22);
    }
}


void parseArgLine(int argc, char* argv[],  char* blokStr,int* mainFd, int* achievementsFd, int* reportsFd, unsigned int* wolumen,
                                                                                unsigned int* blok, unsigned int* prac){
    int option;
    opterr = 0;

    while((option = getopt(argc, argv, "d:s:w:f:l:p:")) != -1){
        switch(option){
            case 'd':
                *mainFd = openFile(optarg, 0);
                break;
            case 's':
                *wolumen = 2*argCheckUnit(optarg);
                break;
            case 'w':
                *blok = argCheckUnit(optarg);
                if(snprintf(blokStr, 10, "%s", optarg) == -1){
                    perror("ERROR using snprintf (parseArgLine)");
                    exit(22);
                }
                break;
            case 'f':
                *achievementsFd = openFile(optarg, 2);
                fillFile(*achievementsFd);
                break;
            case 'l':
                *reportsFd = openFile(optarg, 1);
                break;
            case 'p':
                *prac = argCheck(optarg);
                break;
            case '?':
				fprintf(stderr, "Invalid flag passed: %c \n", optopt);
				exit(22);
        }
    }
    if(*mainFd == -1 || *wolumen == -1 || *blok == -1 ||  *achievementsFd == -1 || *reportsFd == -1 || *prac == -1){
        fprintf(stderr, "Not all flags were passed!\n");
		exit(22);
    }

    if(optind < argc){
        fprintf(stderr, "Too many args passed!\n");
		exit(22);
    }
}      

void writeSuccessRecord(int fd, struct successRecord* record){
    if(lseek(fd, record->number*sizeof(pid_t), SEEK_SET) == -1){
        perror("ERROR using lseek");
        exit(22);
    }

    pid_t pid = 0;
    if(read(fd, &pid, sizeof(pid_t)) != sizeof(pid_t)){
        perror("ERROR using read (writeSuccessRecord)");
        exit(22);
    }

    if(pid == 0){
        if(lseek(fd, record->number*sizeof(pid_t), SEEK_SET) == -1){
            perror("ERROR using lseek");
            exit(22);
        }
        if(write(fd, &(record->pid), sizeof(pid_t)) != sizeof(pid_t)){
            perror("ERROR writing a pid to success file");
            exit(22);
        }
        counter++;
    }
}        

int readSuccessRecord(int writeFd){
    struct successRecord readRecord;
        switch(read(readingPipe[0], &readRecord, sizeof(successRecord))){
            case -1: 
                if(errno == EAGAIN){
                    if(childrenCounter == 0)
                        exit(0);
                    return 0;
                }
                else{
                    perror("ERROR reading a success record");
                    exit(22);
                }
            case 0:
                fprintf(stderr, "EOF\n");
                exit(22);
            default:
                writeSuccessRecord(writeFd, &readRecord); 
                return 1;
        }
}

void writeRaportRecord(int fd, int reason, pid_t pid){
        char buffer[60] = {'\0'};
        char* format1 = "pid:%dsec:%ldnsec:%ldreason:created";
        char* format2 = "pid:%dsec:%ldnsec:%ldreason:died";

        struct timespec time;
        if(clock_gettime(CLOCK_MONOTONIC, &time) == -1){
                perror("Error reading a clock");
                exit(22);
        }
    
        int snprintfRes;
        switch(reason){
                case 0:
                        snprintfRes = snprintf(buffer, 60, format1, pid, time.tv_sec, time.tv_nsec);
                        break;
                case 1:
                        snprintfRes = snprintf(buffer, 60, format2, pid, time.tv_sec, time.tv_nsec);
                        break;
                default:
                        fprintf(stderr, "wrong arg passed! (writeReportRecord2)\n");
                        exit(22);
        }
        if(snprintfRes == -1){
             perror("snprintf error");
             exit(22);
        }
  
        if(write(fd, buffer, snprintfRes) != snprintfRes){
                perror("error write");
                exit(22);
        }
}

int checkDeadChildren(char* blokString, int reportsFd){
    pid_t   pid = -1;
    int     status;  
    int inLoop = 0;
    
    while ( (pid = waitpid(-1, &status, WNOHANG)) > 0) {
        childrenCounter--;
        inLoop = 1;
        writeRaportRecord(reportsFd, 1, pid);
        if(WEXITSTATUS(status) >= 0 && WEXITSTATUS(status) <= 10 && counter < N){
                pid = fork();
                switch(pid){
                    case -1:
                        perror("Error creating a child (checkDeadChildren)");
                        exit(22);
                    case 0:
                        child_do(blokString); 
                        break;
                    default:
                        childrenCounter++;
                        writeRaportRecord(reportsFd, 0, pid);
                        break; 
                }
        }
    }
    return inLoop;
}
