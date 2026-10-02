#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int Socket = 0;

int ServerStart(void) {
  Socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (Socket < 0) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in Address = {0};

  Address.sin_family = AF_INET;
  Address.sin_addr.s_addr = INADDR_ANY;
  Address.sin_port = htons(7777);

  if (bind(Socket, (struct sockaddr *)&Address, sizeof(Address)) < 0) {
    perror("bind");
    close(Socket);
    return 1;
  }

  printf("[Vcgs] Vortex game server started\n");

  uint8_t Buffer[4096];

  bool IsFirstPacket = false;
  while (1) {
    struct sockaddr_in ClientAddress = {0};
    socklen_t ClientLength = sizeof(ClientAddress);

    ssize_t ReceivedLength =
        recvfrom(Socket, Buffer, sizeof(Buffer), 0,
                 (struct sockaddr *)&ClientAddress, &ClientLength);

    if (ReceivedLength < 0) {
      perror("recvfrom");
      break;
    }

    uint32_t PacketType = *(uint32_t *)Buffer;
    if (PacketType == 0x06) {
      const char *Token = (const char *)(Buffer + 12);

      unsigned char Response[18] = {0x00};
      Response[0] = 0x12;
      sendto(Socket, Response, 18, 0, (struct sockaddr *)&ClientAddress,
             ClientLength);
      printf("[Vcgs] Client connected: %s\n",
             inet_ntoa(ClientAddress.sin_addr));
    } else if (PacketType == 0x00) {
      if (IsFirstPacket) {
        IsFirstPacket = true;
      }
    }
  }

  close(Socket);

  return 0;
}
