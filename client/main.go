package main

import (
	"flag"
	"fmt"
	"net"
	"os"
	"time"
)

func sendReq(addr string, id int, n int, results chan []time.Duration) {
	out := []byte("abcdefghijklmnopqrstuvwxyz")
	in := make([]byte, 8192)
	conn, err := net.Dial("tcp", addr)
	if err != nil {
		fmt.Fprintf(os.Stderr, "error: %v\n", err)
		return
	}
	defer conn.Close()
	sum := time.Duration(0)
	times := make([]time.Duration, n)
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
		times[i] = end.Sub(start)
		sum += times[i]
	}
	results <- times
}

func main() {
	var addr string
	flag.StringVar(&addr, "s", "localhost:55443", "server address")
	var requests int
	flag.IntVar(&requests, "r", 10, "number of requests per connection")
	var concurrency int
	flag.IntVar(&concurrency, "c", 100, "number of concurrent connections")
	flag.Parse()
	results := make(chan []time.Duration, concurrency)
	for i := 0; i < concurrency; i++ {
		go sendReq(addr, i, requests, results)
	}
	sum := time.Duration(0)
	for i := 0; i < concurrency; i++ {
		r := <-results
		for j := 0; j < len(r); j++ {
			sum += r[j]
		}
	}
	totalReq := int64(concurrency * requests)
	avgTime := sum / time.Duration(totalReq)
	fmt.Printf("average time/request: %v\n", avgTime)
	sumInSec := int64(sum.Seconds())
	if sumInSec == 0 {
		fmt.Printf("requests/second: N.A.\n")
		return
	}
	reqPerSec := totalReq / sumInSec
	fmt.Printf("requests/second: %v\n", reqPerSec)
}
