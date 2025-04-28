# Chat Application
This command-line program is designed for server-client communication
It utilizes key concepts for efficient  concurrency management.
The primary techniques used are:

Socket Programming – This enables direct communication between the server and clients using network sockets.

Threadpool – To enhance performance, a thread pool mechanism is used to manage multiple client connections efficiently. Instead of creating a new thread for each request.

Non-blocking I/O – The program implements non-blocking operations to ensure that network communication does not halt other processing tasks.
