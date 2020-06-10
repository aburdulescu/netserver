package main

import (
	"flag"
	"fmt"
	"net"
	"os"
	"sort"
	"time"
)

func sendReq(addr string, id int, n int, results chan []time.Duration) {
	out := []byte("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz")
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

type ResultSlice []int

func (r ResultSlice) Len() int           { return len(r) }
func (r ResultSlice) Swap(i, j int)      { r[i], r[j] = r[j], r[i] }
func (r ResultSlice) Less(i, j int) bool { return r[i] < r[j] }

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
	var data ResultSlice
	sum := time.Duration(0)
	for i := 0; i < concurrency; i++ {
		r := <-results
		for j := 0; j < len(r); j++ {
			sum += r[j]
			data = append(data, int(r[j]))
		}
	}
	sort.Sort(data)
	fmt.Println("fastest response time:", time.Duration(data[0]))
	fmt.Println("slowest response time:", time.Duration(data[len(data)-1]))
	percentile50 := int(50.0 / 100.0 * float64(len(data)))
	percentile50Sum := 0
	for i := 0; i < percentile50; i++ {
		percentile50Sum += data[i]
	}
	fmt.Println("50th percentile:", time.Duration(percentile50Sum/percentile50))
	percentile95 := int(95.0 / 100.0 * float64(len(data)))
	percentile95Sum := 0
	for i := 0; i < percentile95; i++ {
		percentile95Sum += data[i]
	}
	fmt.Println("95th percentile:", time.Duration(percentile95Sum/percentile95))
	percentile99 := int(99.0 / 100.0 * float64(len(data)))
	percentile99Sum := 0
	for i := 0; i < percentile99; i++ {
		percentile99Sum += data[i]
	}
	fmt.Println("99th percentile:", time.Duration(percentile99Sum/percentile99))
	totalReq := int64(concurrency * requests)
	avgTime := sum / time.Duration(totalReq)
	fmt.Println("average time/request:", avgTime)
	sumInSec := int64(sum.Seconds())
	if sumInSec == 0 {
		fmt.Println("requests/second: N.A.")
		return
	}
	reqPerSec := totalReq / sumInSec
	fmt.Println("requests/second:", reqPerSec)
}
