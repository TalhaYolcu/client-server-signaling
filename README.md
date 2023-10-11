# client-server-signaling

## Usage

### Dependencies

libdatachannel library is used. First clone libdatachannel repository and build it properly. 

After building it, [libdatachannel.so](http://libdatachannel.so) file should be generated in your /usr/local/lib directory. If not, after cmake and make commands, enter this command inside the directory where you have makefile

```bash
sudo make install
```

This commands makes sure libraries are installed to your system

### Additional Dependencies

After libdatachannel is generated, you have to build also its dependencies. Inside libdatachannel/deps directory . libjuice and libsrtp libraries should be builded also.

You can do the same thing to this libraries. Enter a library directory you want to build and type these commands

```bash
cmake -B build
cd build
make
sudo make install
```

After these commands, library files (.so) should generated and be in the /usr/local/lib directory.

You have to make sure that your ld is updated with this command. If you don’t enter this command, your linker can’t see newly added libraries.

```bash
sudo ldconfig
```

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

Bu program aynı internete bağlı olan client ların SDP lerinin birbirlerine ulaştırması için tasarlanmıştır. İleriki aşamalarda client lar birbirleri ile arada server olmadan videolu ve sesli iletişim kurabileceklerdir.

Server tarafı bu komut ile çalıştırılır

```bash
./server <port number>
```

Client  tarafı bu komut ile çalıştırılır

```bash
./client <server-ip> <server-port> <client-ip> <client-port>
```

1. Server ayakta
2. While içinde accept ile yeni bağlanan clientları bekleyecek
3. Client connect ile server a bağlandı
4. Client için yeni thread açıldı, client ile iletişim o threadde sağlanacak
5. Client öncelikle login signup logout ve exit özelliklerine sahip olacak
6. Offerer client call isteği ile birlilkte arayacağı kişinin username ve kendi sdp sini (offer gönderecek)
7. Server client ın sdp sini alacak, aranan kişiye kendi sdpsini(server in sdpsini) (offer) gönderecek. Eğer aranan kişi aramayı kabul ederse server a answer sdp sini gönderecek. Bundan sonra da server offerer client a server sdpsini (answer) gönderecek.





