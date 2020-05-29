package main

import (
	"io"
	"log"
	"net"
)

func main() {
	l, err := net.Listen("tcp", ":55443")
	if err != nil {
		log.Fatal(err)
	}
	for {
		c, err := l.Accept()
		if err != nil {
			log.Fatal(err)
		}
		go onConnection(c)
	}
}

func onConnection(c net.Conn) {
	defer c.Close()
	buf := make([]byte, 8192)
	for {
		n, err := c.Read(buf)
		if err == io.EOF {
			return
		}
		if err != nil {
			log.Println(err)
			return
		}
		_, err = c.Write(buf[:n])
		if err != nil {
			log.Println(err)
			return
		}
	}
}
