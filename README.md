# Multi-threaded game

Game created as a project for Operating Systems 2 subject. The goal was to learn IPC mechanisms and threads.

The IPC mechanism used in the game are named pipes (FIFO). Thread synchronisation is done by mutexes.

---

To run the game you have to make both server and client. Then execute them respectively.
I have tested it on WSL2, which somehow needs these two commands below to be executed before. Otherwise the FIFO files won't be created thus client and server won't connect.

---
cd ~ && sudo umount /mnt/c

sudo mount -t drvfs C:\\ /mnt/c -o metadata
