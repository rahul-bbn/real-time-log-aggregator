# Real-Time Log Aggregator
---

# Project Overview

The **Real-Time Log Aggregator** is a Linux-based system designed to monitor multiple log files, detect new appended entries, process them asynchronously, and output all logs in perfectly sorted chronological order.

This project showcases real-world systems programming techniques including:

* File system event monitoring using **inotify**
* Asynchronous I/O using **POSIX AIO** or **io_uring**
* Timestamp extraction and parsing
* Multi-threaded synchronization
* Global stream merging using a min‑heap priority queue

---

# Key Features

### 1. Real-Time File Watching

The aggregator continuously monitors multiple log files. Whenever new content is appended, the watcher immediately triggers asynchronous processing.

### 2. Asynchronous Log Reading (POSIX AIO or io_uring)

Instead of blocking reads, the system performs **non‑blocking asynchronous reads**, allowing simultaneous ingestion from multiple log sources.

### 3. Timestamp-Based Merging

Every log line is parsed for its timestamp. The merge engine maintains a global ordering across all files using a **min‑heap**, ensuring logs appear in strict chronological order.

### 4. Multi-threaded Architecture

The system uses several cooperating threads:

* Watcher thread (file change detection)
* Multiple AIO/io_uring completion threads
* Merge engine thread (sorted output)


### 5. Simple Log Format

Each log entry is expected to begin with:

```
YYYY-MM-DD HH:MM:SS message text...
```

This is similar to common formats used by services, containers, and backend applications.

---

# Directory Structure

A typical project layout looks like:

```
real-time-log-aggregator/
 ├── include/
 ├── src/
 ├── logs/
 │    ├── app1.log
 │    └── app2.log
 └── README.md
```

You must manually create the logs directory and empty log files before running the program:

```
mkdir -p logs
```

```
touch logs/app1.log logs/app2.log
```

---

# Building the Project

## Example: Build with io_uring

For systems using io_uring, an example command looks like:

```
gcc -Iinclude -pthread -O2 \
    src/file-watcher.c src/log-reader.c src/timestamp-parser.c \
    src/merge-engine.c src/iouring-helper.c src/main.c \
    -o aggregator -luring
```

---

# Running the Aggregator

Start the application:

```
./aggregator
```

Output:

```
Merge engine running...
Write lines like: 2025-12-03 16:00:01 message to logs/app1.log
[WATCHER] File changed: logs/app2.log
[DEBUG] schedule_read_new_logs called for logs/app2.log
[MERGED] 2025-12-03 16:00:06 App2 world
```

This confirms that:

* file changes were detected
* async read was executed
* merge engine printed sorted entries

---

# Testing the System

Open another terminal window and append log lines manually:

```
echo "2025-12-03 16:00:01 App1 hello" >> logs/app1.log
```

```
echo "2025-12-03 16:00:06 App2 world" >> logs/app2.log
```

The aggregator will immediately print:

```
[MERGED] 2025-12-03 16:00:01 App1 hello
[MERGED] 2025-12-03 16:00:06 App2 world
```

All entries appear in **global timestamp order**, no matter which file they originate from.

---

# Example End-to-End Workflow

### Terminal 1 — Run the aggregator

```
./aggregator
```

### Terminal 2 — Write logs

```
echo "2025-12-03 16:00:01 A startup" >> logs/app1.log

echo "2025-12-03 16:00:03 B init"    >> logs/app2.log

echo "2025-12-03 16:00:04 A ready"   >> logs/app1.log

echo "2025-12-03 16:00:06 B world"   >> logs/app2.log
```

The program outputs:

```
[MERGED] 2025-12-03 16:00:01 A startup
[MERGED] 2025-12-03 16:00:03 B init
[MERGED] 2025-12-03 16:00:04 A ready
[MERGED] 2025-12-03 16:00:06 B world
```

---

# Useful Commands

Copy from a mounted drive:

```
cp -r /mnt/d/real-time-log-aggregator ~/real-time-log-aggregator
```

Move into project folder:

```
cd ~/real-time-log-aggregator
```