# Contents and usage

Use Linux (or WSL), GCC, and Python 3. Run the added checks from the repository root:

```sh
python3 tests/run_checks.py
```

| Directory | Focus | Build / notes |
|---|---|---|
| `file-copy` | File descriptors and buffered I/O | `gcc file-copy/copy.c -o /tmp/course-copy`; `/tmp/course-copy input output 4096` requires an existing output file. |
| `page-table` | Five-level page-table update/query | `pt.c` uses `os.h` and supplied OS simulation support. Tests provide a small simulated physical-memory harness. |
| `shell` | Processes, signals, redirection and pipes | `gcc shell/shell.c shell/myshell.c -o /tmp/course-shell`; launch `/tmp/course-shell`. |
| `message-slot` | Linux character-device module and userspace clients | `make -C message-slot` requires matching kernel headers. Load/testing requires a disposable Linux VM; the automated tests never load a module. |
| `concurrent-queue` | Blocking FIFO with C11 threads | Tests compile `queue.c` with a separate harness and `-pthread`. |
| `unfinished/tcp-character-counter` | Socket-programming draft and supplied client | Incomplete: the server has an undefined variable and does not implement the client's protocol. Not included among successful builds. |

The copy utility does not truncate a longer destination and has limited handling of invalid input/partial writes. The queue's `visited()` reads shared state without its mutex; do not infer thread-safety from the sequential smoke check. `destroyQueue()` requires further review, especially with waiting threads.
