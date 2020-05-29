# netserver
Attempt to write a high-performance network server in C using POSIX API.

## goserver
Echo server written in Go.

Used as baseline for comparing the performance of `cserver`.

## cserver
Echo server written in C using `epoll` and `SO_REUSEPORT` feature. 

## client
Client used for benchmarking the servers.
