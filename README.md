# TP1-SO

This project is designed to generate MD5 hashes for a set of input files by delegating the task to multiple child processes. The project uses shared memory and semaphores for interprocess communication (IPC) between the main process and the child processes.

Compiling instructions

```bash
make all

```

To execute program without view process:

```bash 
./<path_to_md5>/md5 <files_for_hashing>

```

To execute program with view process:

```bash 
<path_to_md5>/md5 <files_for_hashing> | ./<path_to_view>/view
```

Or, with two terminals open run

TTY1:
```bash 
<path_to_md5>/md5 <files_for_hashing>

```
TTY2:
```bash 
    <path_to_view>/view shared_memory_name

```
! The shared memory name is printed by the md5 file in terminal 

To remove generated files:

```bash 
make clean all
```
