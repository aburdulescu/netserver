package main

import (
	"fmt"
	"net"
	"os"
	"time"
)

func main() {
	start := time.Now()
	conn, err := net.Dial("tcp", "localhost:55443")
	if err != nil {
		fmt.Fprintf(os.Stderr, "error: %v\n", err)
		os.Exit(1)
	}
	nwrite, err := conn.Write([]byte("hello"))
	if err != nil {
		fmt.Fprintf(os.Stderr, "error: %v\n", err)
		os.Exit(1)
	}
	buf := make([]byte, 8192)
	nread, err := conn.Read(buf)
	if err != nil {
		fmt.Fprintf(os.Stderr, "error: %v\n", err)
		os.Exit(1)
	}
	end := time.Now()
	fmt.Printf("wrote %d, read %d in %v\n", nwrite, nread, end.Sub(start))
}
