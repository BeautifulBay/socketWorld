# This is a module of tcp/IP.
# Epoll fd listen to server fd and stdin and client fd.
# If there is a message from client, read it and enqueue client queue to thread to handle.
# Loop to dequeue and handle client message in thread.
# There are two list and one queue. One list is for all client and another is for client in queue.
# One queue is used to save info of one client.

#server:
./server.o
# You can input q to quit server

#client:
./client.o
./client.o
......
# You can input q to quit client
# You can input other words sent to sersver

