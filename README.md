# client-server-signaling


## Usage

### Build

```bash
make all
```

### Server

give port number, in my case 12345

```bash
./server 12345 
```

### Client

give server ip address, server port, client ip address and client port

```bash
./client 172.20.10.3 12345 172.20.10.4 8080
```