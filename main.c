#include <sys/ptrace.h>
#include <sys/user.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <linux/elf.h>

typedef struct user_regs_struct iregs_struct;

#ifdef __ANDROID__
typedef struct user_fpsimd_struct fregs_struct;
#else
typedef struct user_fpregs_struct fregs_struct;
#endif

int main(void) {
    struct iovec io;

    // pid_t my_pid = getpid();
    iregs_struct iregs = {0};
    fregs_struct fpregs = {0};

    long result;
    size_t ptr_size = sizeof(size_t); 
    size_t iregs_size = sizeof(iregs);
    size_t fpregs_size = sizeof(fpregs);

    pid_t child_pid = fork();
    if (child_pid == -1) {
       printf("failed fork");
       return -1;
   }

    if (child_pid == 0) {
        result = ptrace(PTRACE_TRACEME, NULL, NULL, NULL);
        if (result == -1) {
            printf("failed traceme?! %li!\n", result);
            return result;
        }
        pid_t child_tid = getpid();
        printf("child pid %d\n", child_tid);
        __builtin_trap();

        pause();
        printf("wait fuck\n");
        return 0;
    }
    printf("child pid %d\n", child_pid);

    int wstatus;
    do {
        pid_t w = waitpid(child_pid, &wstatus, WUNTRACED | WCONTINUED);
        if (w == -1) {
           perror("waitpid");
           exit(EXIT_FAILURE);
        }

        if (WIFEXITED(wstatus)) {
           printf("exited, status=%d\n", WEXITSTATUS(wstatus));
        } else if (WIFSIGNALED(wstatus)) {
           printf("killed by signal %d\n", WTERMSIG(wstatus));
        } else if (WIFSTOPPED(wstatus)) {
           printf("stopped by signal %d\n", WSTOPSIG(wstatus));
        } else if (WIFCONTINUED(wstatus)) {
           printf("continued\n");
        }
    } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus) && !WIFSTOPPED(wstatus));

    printf("time to go!\n");

    io.iov_base = &iregs;
    io.iov_len = sizeof(iregs);
    result = ptrace(PTRACE_GETREGSET, child_pid, (void*)NT_PRSTATUS, &io);
    if (result == -1) {
        printf("failed getregset %li!\n", result);
        return result;
    } else {
        printf("getregset success (len %zu, size %zu, ptr_size %zu)!\n", io.iov_len, iregs_size, ptr_size);
    }

#ifdef NT_PRFPREG
    io.iov_base = &fpregs;
    io.iov_len = sizeof(fpregs);
    result = ptrace(PTRACE_GETREGSET, child_pid, (void*)NT_PRFPREG, &io);
    if (result == -1) {
        printf("failed getregset %li!\n", result);
        return result;
    } else {
        printf("getregset success (len %zu, size %zu, ptr_size %zu)!\n", io.iov_len, fpregs_size, ptr_size);
    }
#endif

/*
    result = ptrace(PTRACE_GETREGS, child_pid, 0, &iregs);
    if (result == -1) {
        printf("failed getregs %li!\n", result);
        return result;
    } else {
        printf("getregs success (size %zu, ptr_size %zu)!\n", iregs_size, ptr_size);
    }
    result = ptrace(PTRACE_GETFPREGS, child_pid, 0, &fpregs);
    if (result == -1) {
        printf("failed getfpregs %li!\n", result);
        return result;
    } else {
        printf("getfpregs success (size %zu, ptr_size %zu)!\n", fpregs_size, ptr_size);
    }
*/

    printf("\niregs:\n");
    {
        char* reader = (char*)&iregs;
        for (size_t i=0; i<iregs_size / ptr_size ; i+=1) {
            size_t offset = (i * ptr_size);
            size_t val = 0;
            memcpy((char*)&val, reader + offset, ptr_size);

            printf("%zx: %zx\n", offset, val);
        }
    }
    printf("\nfpregs:\n");
    {
        char* reader = (char*)&fpregs;
        for (size_t i=0; i<fpregs_size / ptr_size ; i+=1) {
            size_t offset = (i * ptr_size);
            size_t val = 0;
            memcpy((char*)&val, reader + offset, ptr_size);

            printf("%zx: %zx\n", offset, val);
        }
    }

    return 0;
}