#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <format>
#include <print>
#include "strace.hpp"

/*
fork a child process that will be traced using ptrace
the child process will execute the program specified in argv[1]
the parent process will wait for the child to stop and then return the child's pid
*/
pid_t fork_traced_child(const char **argv) {
    pid_t child_pid = fork(); // Create a new process

    if (child_pid == 0) {
        // Child process

        // Allow tracing of this process
        ptrace((enum __ptrace_request)PTRACE_TRACEME, 0, 0, 0);

        int res = execvp(argv[1], (char * const *)&argv[1]);

        // If execvp returns, there was an error
        std::print("Failed to exec {} with error {}\n", argv[1], res);
        _exit(1);

    } else if (child_pid > 0) {
        // Parent process
        int status;
        waitpid(child_pid, &status, 0);// Wait for child to stop on its first instruction
        if (!WIFSTOPPED(status)) {
            std::print("Child process did not stop as expected!\n");
            return -1;
        }
        std::print("Child process {} has stopped, ready for tracing.\n", child_pid);
        // Set ptrace options so we can get syscall info easily
        ptrace((enum __ptrace_request)PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACESYSGOOD);
        // Now the parent can trace the child

        return child_pid;
    } else {
        // Fork failed
        perror("fork");
        return -1;
    }
}


inline constexpr int MAX_SYSCALL_ARGS = 6;

template<int N>
std::string format_syscall_args(const struct ptrace_syscall_info &info) {
    
    std::string_view args_str = syscall_aritary_map[info.entry.nr].second;
    
    if constexpr (N == 0) {
        return "";
    }

    return std::vformat(args_str, [&]<std::size_t... Is>
    (std::index_sequence<Is...> _) {
        return std::make_format_args(info.entry.args[Is]...);
    }(std::make_index_sequence<N>{}));
}

auto (*formatters[MAX_SYSCALL_ARGS + 1])(const struct ptrace_syscall_info &info) -> std::string = {
    format_syscall_args<0>,
    format_syscall_args<1>,
    format_syscall_args<2>,
    format_syscall_args<3>,
    format_syscall_args<4>,
    format_syscall_args<5>,
    format_syscall_args<6>,
};

void trace_child_process(pid_t child_pid) {
    int status;
    while (true) {
        std::string command = "";
    
        // Let the child run until next syscall entry
        ptrace((enum __ptrace_request)PTRACE_SYSCALL, child_pid, 0, 0); 
        waitpid(child_pid, &status, 0);

        // it should have stopped on syscall exit
        // if it didnt something went wrong
        if (!WIFSTOPPED(status))
        {
            std::print("Child process had critical error\n");
            return;
        }
        
        struct ptrace_syscall_info info;
        ptrace((enum __ptrace_request)PTRACE_GET_SYSCALL_INFO, child_pid, sizeof(info), &info);// Get syscall entry info

        command += syscall_name_map[info.entry.nr];
        int arity = syscall_aritary_map[info.entry.nr].first;

        std::string args_str;
        if(arity == -1) {
            args_str = "...";
        }
        else {
            args_str = formatters[arity](info);
        }
        command += std::format("({})", args_str);

        // Now let the child continue to the syscall exit
        ptrace((enum __ptrace_request)PTRACE_SYSCALL, child_pid, 0, 0);
        waitpid(child_pid, &status, 0);

        // Check if the child has exited
        // as it may exit during syscall
        if(WIFEXITED(status)) {
            std::print("{}\n", command);
            std::print("Child process {} has exited.\n", child_pid);
            break;
        }
        // it should have stopped on syscall exit
        // if it didnt something went wrong
        if (!WIFSTOPPED(status))
        {
            std::print("Child process did not stop as expected!\n");
            return;
        }
        ptrace((enum __ptrace_request)PTRACE_GET_SYSCALL_INFO, child_pid, sizeof(info), &info);

        command += std::format(" = {}", info.exit.rval);

        std::print("{}\n", command);
    }

}



int main(const int argc, const char **argv) {
    if (argc == 1)
    {
        std::print("Usage: {} <program> [args...]\n", argv[0]);
        return 1;
    }

    pid_t child_pid = fork_traced_child(argv);
    trace_child_process(child_pid);
    return 0;
}
