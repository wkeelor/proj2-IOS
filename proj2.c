#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <regex.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>

#define WRITE_FILE "sem_write"
#define O_QUEUE "sem_o_queue"
#define H_QUEUE "sem_h_queue"
#define O_ALLOW "sem_o_allow"
#define H_ALLOW "sem_h_allow"
#define MOLECULE "sem_mol"
#define MUTEX "sem_mutex"

int cnt_id = 0, *cnt = NULL;
int mol_id = 0, *mol = NULL;
FILE *proj2out;

sem_t *write_file;
sem_t *oxygen_queue;
sem_t *hydrogen_queue;
sem_t *oxygen_allow;
sem_t *hydrogen_allow;
sem_t *molecule;
sem_t * mutex;

void errprint(const char *string);
void control_arg(int argc, char **argv);
void file();


void oxygen(int i, int TI, int TB);

void start(const char *string, int i);

void init_shm();

void init_sem();

void unlink_();

void hydrogen(int i, int TI);
void queue(const char *string, int i);

void creation(const char *string, int i);

int main(int argc, char **argv){
    setbuf(stdout,NULL);
    setbuf(stderr,NULL);
    unlink_();
    control_arg(argc,argv);
    init_shm();
    init_sem();
    file();
    setbuf(proj2out,NULL);

    int NO = atoi(argv[1]);
    int NH = atoi(argv[2]);
    int TI = atoi(argv[3]);
    int TB = atoi(argv[4]);

    for( int i = NO; i > 0;i--){
        pid_t pid = fork();
        if(pid == 0){
            oxygen(i, TI, TB);
        }else if(pid == -1){
            errprint("Error fork!");
        }
    }

    for( int i = NH; i > 0; i--){
        pid_t pid = fork();
        if(pid == 0){
            hydrogen(i, TI);
        }else if(pid == -1){
            errprint("Error fork!");
        }
    }

    for(int i=NO;i>NO;i--){
        wait(NULL);
    }
    for(int i=NH;i>NH;i--){
        wait(NULL);
    }
    unlink_();
    fclose(proj2out);
    return 0;
}

void unlink_() {
    sem_close(write_file);
    sem_unlink(WRITE_FILE);
    sem_close(oxygen_queue);
    sem_unlink(O_QUEUE);
    sem_close(hydrogen_queue);
    sem_unlink(H_QUEUE);
    sem_close(oxygen_allow);
    sem_unlink(O_ALLOW);
    sem_close(hydrogen_allow);
    sem_unlink(H_ALLOW);
    sem_close(molecule);
    sem_unlink(MOLECULE);
    sem_close(mutex);
    sem_unlink(MUTEX);
    shmdt(cnt);
    shmdt(mol);
}

void init_sem() {
    if((write_file = sem_open(WRITE_FILE,O_CREAT | O_EXCL,0666,1)) == SEM_FAILED){
        errprint("Error semaphore write_file!");
    }
    if((oxygen_queue = sem_open(O_QUEUE,O_CREAT | O_EXCL,0666,0)) == SEM_FAILED){
        errprint("Error semaphore oxygen_queue!");
    }
    if((hydrogen_queue = sem_open(H_QUEUE,O_CREAT | O_EXCL,0666,0)) == SEM_FAILED){
        errprint("Error semaphore hydrogen_queue!");
    }
    if((oxygen_allow = sem_open(O_ALLOW,O_CREAT | O_EXCL,0666,1)) == SEM_FAILED){
        errprint("Error semaphore oxygen_allow!");
    }
    if((hydrogen_allow = sem_open(H_ALLOW,O_CREAT | O_EXCL,0666,2)) == SEM_FAILED){
        errprint("Error semaphore hydrogen_allow!");
    }
    if((molecule = sem_open(MOLECULE,O_CREAT | O_EXCL,0666,3)) == SEM_FAILED){
        errprint("Error semaphore molecule!");
    }
    if((mutex = sem_open(MUTEX, O_CREAT | O_EXCL,0666,1)) == SEM_FAILED){
        errprint("Error semaphore mutex!");
    }
}

void init_shm() {
    cnt_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if(cnt_id == -1){
        errprint("Error shared memory!");
    }
    cnt = (int *) shmat(cnt_id, NULL, 0);
    if(cnt == NULL){
        errprint("Error shared memory pointer!");
    }
    mol_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if(mol_id == -1){
        errprint("Error shared memory!");
    }
    mol = (int *) shmat(mol_id, NULL, 0);
    if(mol == NULL){
        errprint("Error shared memory pointer!");
    }
    *mol = 1;
}

void creation(const char *string, int i) {
    int o,h;
    while(1){
        sem_getvalue(oxygen_queue,&o);
        sem_getvalue(hydrogen_queue,&h);
        if(o >= 1 && h >= 2) {
            sem_wait(write_file);
            fprintf(proj2out, "%d: %s %d: creating molecule %d\n", ++*cnt, string, i, *mol);
            sem_post(write_file);
            break;
        }
    }
}

void oxygen(int i, int TI, int TB) {
    int o,h,m;
    start("O",i);
    usleep(rand() % TI+1);
    queue("O", i);
    sem_post(oxygen_queue);
    sem_getvalue(oxygen_allow,&o);
    sem_getvalue(hydrogen_allow,&h);
    if(o != 0) {
        sem_wait(oxygen_allow);
        sem_wait(oxygen_queue);
        sem_wait(molecule);
        creation("O", i);
        while (1) {
            sem_getvalue(molecule, &m);
            sem_wait(mutex);
            if (m == 0) {
                sem_getvalue(oxygen_queue, &o);
                sem_getvalue(hydrogen_queue, &h);
                usleep(rand() % TB + 1);
                sem_post(molecule);
                sem_wait(write_file);
                fprintf(proj2out, "%d: O %d: molecule %d created\n", ++*cnt, i, *mol);
                sem_post(write_file);
                sem_post(oxygen_allow);
                while(1){
                    sem_getvalue(hydrogen_allow,&h);
                    if(h == 2){
                        *mol += 1;
                        sem_post(molecule);
                        sem_post(molecule);
                        sem_post(mutex);
                        break;
                    }
                }
                break;
            }else{
                sem_post(mutex);
            }
        }
    }
    exit(0);
}

void hydrogen(int i, int TI) {
    int h;
    start("H",i);
    usleep(rand() % TI+1);
    queue("H", i);
    sem_post(hydrogen_queue);
    sem_getvalue(hydrogen_allow,&h);
    if(h != 0){
        sem_wait(hydrogen_allow);
        sem_wait(hydrogen_queue);
        sem_wait(molecule);
        creation("H", i);
        sem_wait(molecule);
        sem_wait(write_file);
        fprintf(proj2out,"%d: H %d: molecule %d created\n", ++*cnt, i, *mol);
        sem_post(write_file);
        sem_post(hydrogen_allow);
        sem_post(molecule);
    }
    exit(0);
}

void queue(const char *string, int i) {
    sem_wait(write_file);
    fprintf(proj2out,"%d: %s %d: going to queue\n", ++*cnt, string, i);
    sem_post(write_file);
}

void start(const char *string, int i) {
    sem_wait(write_file);
    fprintf( proj2out,"%d: %s %d: started\n", ++*cnt, string, i);
    sem_post(write_file);
}

void file() {
    proj2out = fopen("proj2.out","w+");
    if(proj2out == NULL){
        errprint("Error opening/creating file 'proj2.out'!\n");
    }
}

void control_arg(int argc, char **argv) {
    if(argc != 5){
        errprint("Invalid number of arguments!");
    }
    regex_t regex;
    int ret_val = regcomp(&regex,"^.*[^0-9].*$",0);
    if(ret_val != 0){
        errprint("Error compiling regex!");
    }
    for(int i=1;i<5;i++){
        ret_val = regexec(&regex,argv[i],0,NULL,0);
        if(ret_val == 0){
            errprint("Invalid argument!");
        }
    }
}

void errprint(const char *string) {
    fprintf(stderr,"%s",string);
    exit(EXIT_FAILURE);
}