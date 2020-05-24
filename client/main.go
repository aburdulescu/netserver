package main

import (
	"flag"
	"fmt"
	"net"
	"os"
	"sync"
	"time"
)

func sendReq(wg *sync.WaitGroup, id int, n int) {
	defer wg.Done()
	out := []byte("abcdefghijklmnopqrstuvwxyz")
	in := make([]byte, 8192)
	conn, err := net.Dial("tcp", "localhost:55443")
	if err != nil {
		fmt.Fprintf(os.Stderr, "error: %v\n", err)
		return
	}
	defer conn.Close()
	for i := 0; i < n; i++ {
		start := time.Now()
		_, err = conn.Write(out)
		if err != nil {
			fmt.Fprintf(os.Stderr, "error: %v\n", err)
			return
		}
		_, err = conn.Read(in)
		if err != nil {
			fmt.Fprintf(os.Stderr, "error: %v\n", err)
			return
		}
		end := time.Now()
		fmt.Printf("[%d] %d %v\n", id, i, end.Sub(start))
	}
}

func main() {
	var requests int
	flag.IntVar(&requests, "r", 10, "number of requests per connection")
	var concurrency int
	flag.IntVar(&concurrency, "c", 100, "number of concurrent connections")
	flag.Parse()
	var wg sync.WaitGroup
	for i := 0; i < concurrency; i++ {
		wg.Add(1)
		go sendReq(&wg, i, requests)
	}
	wg.Wait()
}
