file_transfer.c:
    1.Introduce: A socket program with both server's and client's code. The third argument would determine whether you're server or client (4.Usage).
    2.Transport Layer Protocol: TCP | UDP
    3.Compile: gcc file_transfer.c -o file_transfer 
    4.Usage:  server => ./file_transfer <tcp/udp> send <host_ip> <port> <filename>
              client => ./file_transfer <tcp/udp> recv <host_ip> <port>  
    5.Executing first: server

TCP/
multithread.c, client.c:
    1.Introduce: Server code with multi threaded process, Client code is just a normal client socket...
    2.Transport Layer Protocol: TCP
    3.Compile:  server => gcc multithread_server.c -l pthread -o multithread_server
                client => gcc client.c -o client
    4.Usage:  server => ./multithread_server <host_ip> <port> <filename>
              client => ./client <host_ip> <port>
    5.Executing first: server        

UDP/
multicast_server.c, multicast_client.c:
    1.Introduce: Socket program with multicast. Client will calculate the lose packet rate after fully receives packets from server.
                 //CLASS D router's ip address: 226.1.1.1
    2.Transport Layer Protocol: UDP
    3.Compile:  server => gcc multicast_server.c -o multicast_server
                client => gcc multicast_client.c -o multicast_client
    4.Usage:  server => ./multicast_server <host_ip> <port> <filename>
              client => ./multicast_client <host_ip> <port>
    5.Executing first: client

UDP/FEC/
fec_server.cpp, fec_client.cpp: 
    1.Introduce: Multicast socket program with the help of "Forward Error Correction(FEC)". 
                 //args: K=20, N=32
                 //CLASS D router's ip address: 226.1.1.1
    2.Transport Layer Protocol: UDP
    3.Compile:  server => g++ fec_server.cpp *.o -o fec_server
                client => g++ fec_client.cpp *.o -o fec_client
    4.Usage:  server => ./fec_server <host_ip> <port> <filename>
              client => ./fec_client <host_ip> <port>
    5.Executing first: client
    6.Reference: Forward error correction with SIMD optimizations (https://github.com/randombit/fecpp), 
                 where is the .o and .h files in UDP/FEC/ comes from.


