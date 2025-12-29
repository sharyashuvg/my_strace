#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>


pid_t fork_traced_child(const char **argv) {
    pid_t child_pid = fork(); // Create a new process

    if (child_pid == 0) {
        // Child process

        // Allow tracing of this process
        ptrace(PTRACE_TRACEME, 0, 0, 0);

        int res = execvp(argv[1], (char * const *)&argv[1]);

        // If execvp returns, there was an error
        printf("Failed to exec %s with error %d\n", argv[1], res);
        printf("errno: %d\n", errno);
        _exit(1);

    } else if (child_pid > 0) {
        // Parent process
        int status;
        waitpid(child_pid, &status, 0);// Wait for child to stop on its first instruction
        if (!WIFSTOPPED(status)) {
            printf("Child process did not stop as expected!\n");
            return -1;
        }
        printf("Child process %d has stopped, ready for tracing.\n", child_pid);
        // Set ptrace options so we can get syscall info easily
        ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACESYSGOOD);
        // Now the parent can trace the child

        return child_pid;
    } else {
        // Fork failed
        perror("fork");
        return -1;
    }
}


void print_syscall_info(struct ptrace_syscall_info *info) {

    //printf("Syscall Info Operation: %d\n", info->op);

    if (info->op == PTRACE_SYSCALL_INFO_ENTRY) {
        printf("Syscall Entry: number=%lld\n", info->entry.nr);
    } else if (info->op == PTRACE_SYSCALL_INFO_EXIT) {
        printf("Syscall Exit: return value=%lld\n", info->exit.rval);
    } else {
        printf("Unknown syscall info operation: %d\n", info->op);
    }
}


void trace_child_process(pid_t child_pid) {
    int status;
    while (1) {
    
        // Let the child run until next syscall entry
        ptrace(PTRACE_SYSCALL, child_pid, 0, 0); 
        waitpid(child_pid, &status, 0);

        // it should have stopped on syscall exit
        // if it didnt something went wrong
        if (!WIFSTOPPED(status))
        {
            printf("Child process had critical error\n");
            return;
        }
        
        struct ptrace_syscall_info info;
        ptrace(PTRACE_GET_SYSCALL_INFO, child_pid, sizeof(info), &info);// Get syscall entry info
        print_syscall_info(&info);


        // Now let the child continue to the syscall exit
        ptrace(PTRACE_SYSCALL, child_pid, 0, 0);
        waitpid(child_pid, &status, 0);

        // Check if the child has exited
        // as it may exit during syscall
        if(WIFEXITED(status)) {
            printf("Child process %d has exited.\n", child_pid);
            break;
        }
        // it should have stopped on syscall exit
        // if it didnt something went wrong
        if (!WIFSTOPPED(status))
        {
            printf("Child process did not stop as expected!\n");
            return;
        }
        ptrace(PTRACE_GET_SYSCALL_INFO, child_pid, sizeof(info), &info);

        print_syscall_info(&info);

    }

}



int main(const int argc, const char **argv) {
    if (argc == 1)
    {
        printf("Usage: %s <program> [args...]\n", argv[0]);
        return 1;
    }

    pid_t child_pid = fork_traced_child(argv);
    trace_child_process(child_pid);
    return 0;
}
