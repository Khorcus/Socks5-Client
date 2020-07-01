# Socks5-Client

Socks5-Client is a multithreaded asynchronous SOCKS5 client for stress testing [SOCKS5 proxy server](https://github.com/Khorcus/Socks5-Server). It is written in C ++ using the kqueue library.

## Author

It was created by Alexander Khamitov for the subject "Modern technologies of programming and information processing" in HSE (2019-2020)

## Results
Tests were conducted on a Macbook Air 2015 with the following processor model: 1.6 GHz Intel Core i5. In view of this, measurements were made with the following distribution of threads to the client and server: (1,3), (2,2), (3,1).

The number of processed requests was calculated as follows:
1. 100 sockets were opened, each of which, after passing SOCKS5-protocol part, began to send and receive data;
2. "Send -> Receive" chain was considered for one processed request;
3. After the timer was triggered, the total number of processed requests on each thread was summed.

![Results](/images/results.png)
