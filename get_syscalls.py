import subprocess

def get_syscalls():
    """
    Get the list of syscalls from the syscalls.c file using the C preprocessor with inclusion of syshcall.h.
    Returns a list of syscall definitions.
    """
    process = subprocess.Popen(['cpp', '-dM', 'syscalls.c'], stdout=subprocess.PIPE, text=True)
    stdout = process.communicate()[0]
    assert all([line.startswith("#define") for line in stdout.splitlines()])
    return filter(lambda line: line.startswith('__NR_'), map(lambda line: ' '.join(line.strip().split()[1:]), stdout.splitlines()))


def get_aritary_for_syscall(syscall_name):
    """
    get arity for a syscall from man page
    
    :param syscall_name: the man page name of the syscall
    :return: list of argument types or ["..."] if not found
    """
    process = subprocess.Popen(['man', '-P cat', '2', syscall_name], stdout=subprocess.PIPE, text=True)
    stdout = process.communicate()[0]
    stdout = stdout[:stdout.find("DESCRIPTION")] #only look before description at the sypnopsis
    #print(stdout)
    for i in range(len(stdout.splitlines())):
        line = stdout.splitlines()[i]
        if syscall_name not in line: # if the syscall name is not in the line, skip
            continue

        split_line = line.split(f'{syscall_name}(')
        if len(split_line) < 2: # if there is no '(', skip
            continue

        if split_line[0].strip() == '':# make sure there is return type otherwise its explaination text
            continue

        rest = split_line[1]
        if ');' not in split_line[1]:
            if rest.endswith('...'):
                rest = rest.replace('...', '')
            while ');' not in rest:
                i += 1
                if i >= len(stdout.splitlines()):
                    return ["..."]
                rest += stdout.splitlines()[i].replace('...', '').strip()
            
        args = rest[:rest.find(');')].strip().split(',')
        if args == ['void'] or args == ['']:
            args = []
        return args
    
    return ["..."]




def gen_aritary(syscalls):
    """
    Generate a mapping of syscall numbers to their argument types.
    :param syscalls: dict mapping syscall numbers to names
    :return: dict mapping syscall numbers to list of argument types
    """
    aritary = {}
    for num, name in syscalls.items():
        aritary[num] = get_aritary_for_syscall(name)  # Placeholder for actual aritary data
    
    return aritary


def main():
    syscalls = get_syscalls()

    # Create a mapping of syscall numbers to names
    map = {
        (int(syscall.split()[1])): syscall[len('__NR_'):].split()[0] for syscall in syscalls
    }

    #sort the map by syscall numbers
    myKeys = list(map.keys())
    myKeys.sort()
    sorted_map = {i: map[i] for i in myKeys}

    aritary = gen_aritary(sorted_map)

    with open('syscall_list.txt', 'w') as f:
        for num, name in sorted_map.items():
            args = aritary.get(num, ['...'])
            if args == ["..."]:
                arit = -1
            elif args == [] or args == ['void'] or args == ['']:
                arit = 0
            else:
                arit = len(args)
            #save the date to a file in a ugly format(just spaces) for easy parsing later
            f.write(f'"{num} {name} { arit } {" : {}, ".join(args)} : {{}}"  , \n')


if __name__ == "__main__":
    main()
