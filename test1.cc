#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <atomic>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <new>

#define SHM_KEY 0x3887

class SHMObj {
public:
	SHMObj()
		: count(100)
	{}
	void increase() {
		count++; 
		print();
	}
	void decrease() {
		count --;
		print();
	}
	void print() {
		std::cout << "count = " << count << std::endl;
	}
private:
	std::atomic<int> count;
};

static int shm_id = 0;
static void* shm_addr = nullptr;
static SHMObj* shm_object = nullptr;

static int attach(bool server) {
	shm_id = shmget(SHM_KEY, sizeof(SHMObj), server ? (IPC_CREAT | 0660) : 0660);
	if(shm_id < 0) {
		std::cerr << "ERROR: shmget()" << std::hex << SHM_KEY 
			<< " failed, errno = " << errno
			<< " strerror = " << strerror(errno)
			<< std::endl;
		return -1;
	}
	if( (shm_addr = (struct shm_content*) shmat(shm_id, (void*)0, 0)) == nullptr ) {
		std::cerr << "ERROR:attach errno=" << errno
			<< " strerror=" << strerror(errno)
			<< std::endl;
		return -1;
	}
	std::cout << "succ: shmget and shmat for key:" << std::hex << SHM_KEY
		<< std::dec << " shm_id:" << shm_id << " address:" << shm_addr
		<< " object:" << shm_object << std::endl;
}

static int dettach()
{
	if( shmdt(shm_addr) != 0 ) {
		std::cerr << "ERROR: dettach(), errno = " << errno
			<< ", strerror = " << strerror(errno)
			<< std::endl;
		return -1;
	}
	if( shmctl(shm_id, IPC_RMID, nullptr) < 0 ) {
		std::cerr << "ERROR: detach(), errno = " << errno
			<< ", strerror = " << strerror(errno)
			<< std::endl;
		return -1;
	}
	return 0;
}

static int create(bool server) {
    if (server) {
        if ((shm_object = new (shm_addr) SHMObj()) == NULL) {
            printf("ERROR: attach(), errno=[%d],strerror=[%s]\n", errno, strerror(errno));
            return -1;
        }
    }
    else {
        //shm_object = (SHMObj *)(shm_addr);
        shm_object = reinterpret_cast<SHMObj *>(shm_addr);
    }
    printf("SUCC: create(%d), address=[0x%x],object=[0x%x]\n", server, shm_addr, shm_object);
    return 0;
}

static int increase() {
    if (shm_object != NULL) {
        shm_object->increase();
    }
    else {
        printf("ERROR: increase(), call attach firstly\n");
    }
}

static int decrease() {
    if (shm_object != NULL) {
        shm_object->decrease();
    }
    else {
        printf("ERROR: increase(), call attach firstly\n");
    }
}

static int print() {
    if (shm_object != NULL) {
        shm_object->print();
    }
    else {
        printf("ERROR: print(), call attach firstly\n");
    }
}

void help() {
    printf("attach  : \n");
    printf("dettach : \n");
    printf("create  : \n");
    printf("increase: \n");
    printf("decrease: \n");
    printf("print   : \n");
    printf("quit    : quit program\n");
}

int parseCommand(char * cmd, char * argv[]) {
   const char sep[3] = " \n";
   char *token = strtok(cmd, sep);

   int i = 0;
   while (token != NULL) {
      argv[i++] = token;
      token = strtok(NULL, sep);
   }
    return i;
}

int main(int argc, char * argv[]) {
    char cmdbuffer[1024];
    char * cmds[10];    /** max parameters count */
    int i = 0;

    while (1) {
        printf("CMD> ");
        fgets(cmdbuffer, 1024, stdin);
        i = parseCommand(cmdbuffer, cmds);
        if (i > 0) {
            if (strcmp(cmds[0], "quit") == 0) {
                break;
            }
            else if (strcmp(cmds[0], "attach") == 0) {
                    attach(argc > 1 && strcmp(argv[1],"server") == 0);
            }
            else if (strcmp(cmds[0], "dettach") == 0) {
                    dettach();
            }
            else if (strcmp(cmds[0], "create") == 0) {
                    create(argc > 1 && strcmp(argv[1],"server") == 0);
            }
            else if (strcmp(cmds[0], "increase") == 0) {
                    increase();
            }
            else if (strcmp(cmds[0], "decrease") == 0) {
                    decrease();
            }
            else if (strcmp(cmds[0], "print") == 0) {
                    print();
            }
            else if (strcmp(cmds[0], "help") == 0) {
                help();
            }
            else {
                printf("unknown command: %s\n", cmds[0]);
            }
        }
    }
    return 0;
}
