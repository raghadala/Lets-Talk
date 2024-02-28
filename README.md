## Let's Talk 💻

Let's Talk is a multithreaded chat application for Linux that facilitates real-time text-based communication between two users through terminal-to-terminal interaction. This C program utilizes pthreads for concurrent communication via UDP, implementing features such as peer-to-peer communication, effective synchronization, and UDP socket communication.

**Prerequisites**
* Linux machine
* gcc and pthreads installed

**Usage**
* Type make in terminal to compile program
* Type ./s-talk followed by your port number, the other user's hostname, and then their port number.
* The other user will also type ./s-talk followed by their port number, your hostname and then your port number.
* Type ! and Enter to exit the chat
