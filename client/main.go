package main

import (
	"flag"
	"fmt"
	"net"
	"os"
	"sync"
	"time"
)

func sendReq(wg *sync.WaitGroup, i int) {
	defer wg.Done()
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
	fmt.Printf("%d: wrote %d, read %d in %v\n", i, nwrite, nread, end.Sub(start))
}

func main() {
	var concurrency int
	flag.IntVar(&concurrency, "c", 100, "number of concurrent connections")
	flag.Parse()
	var wg sync.WaitGroup
	for i := 0; i < concurrency; i++ {
		wg.Add(1)
		go sendReq(&wg, i)
	}
	wg.Wait()
}
