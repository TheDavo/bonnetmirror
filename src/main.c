#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "./lib/adafruit-oled-bonnet/src/bonnet.h"
#include "lib/adafruit-oled-bonnet/src/ssd1306.h"
#include "lib/adafruit-oled-bonnet/src/ssd1306_gl.h"
#include "lib/adafruit-oled-bonnet/src/uic/segment16.h"

#define PORT "8080"
#define BACKLOG 10

/**
 * send_framebuffer_data sends the framebuffer held by the ssd1306 struct
 * in the bonnet `b` object to the client
 *
 * return the amount of bytes written to the client
 *
 */
int send_framebuffer_data(int clientfd, struct bonnet b);

int main(void) {

  struct ifaddrs *ifaddr, *ifa;

  char ip[INET_ADDRSTRLEN];

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    return -1;
  }

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    // Skip if address is NULL or not IPv4
    if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
      continue;

    // Skip loopback interfaces
    if (ifa->ifa_flags & IFF_LOOPBACK)
      continue;

    // Convert IP address to string
    void *addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
    inet_ntop(AF_INET, addr, ip, sizeof(ip));

    printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, ip);
  }

  int status;
  struct bonnet b;
  status = bonnet_struct_init(&b, 0x3C);
  if (0 != status) {
    perror("bonnet init");
    return (-1);
  }
  bonnet_display_initialize(b);
  bonnet_action_clear_display(&b);

  ssd1306_fb_vec2_t intro_origin = {
      .x = 1,
      .y = 24,
  };

  ssd1306_fb_vec2_t ip_origin = {
      .x = 1,
      .y = 34,
  };

  char *intro_str = "Connect to:";
  ssd1306_fb_draw_8x8font_str(b.ssd.framebuf, intro_origin, intro_str,
                              strlen(intro_str), true, true);
  ssd1306_fb_draw_8x8font_str(b.ssd.framebuf, ip_origin, ip, strlen(ip), true,
                              true);

  ssd1306_write_framebuffer_all(b.ssd);
  sleep(1);
  int sockfd;
  int clientfd;

  struct addrinfo hints;
  struct addrinfo *res;

  // from beej's guide to sockets
  // https://beej.us/guide/bgnet/html/split/system-calls-or-bust.html#system-calls-or-bust
  //

  memset(&hints, 0, sizeof hints);
  // do not care if IPv4 or IPv6
  hints.ai_family = AF_UNSPEC;
  // TCP stream sockets
  hints.ai_socktype = SOCK_STREAM;
  // fill in my IP for me
  hints.ai_flags = AI_PASSIVE;

  if (0 != (status = getaddrinfo(NULL, PORT, &hints, &res))) {
    return (-1);
  }

  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (-1 == sockfd) {
    freeaddrinfo(res);
    perror("opening socket");
    return (-1);
  }
  printf("sockfd creation OK\n");

  // get rid of "Address already in use" error message
  //
  int yes = 1;
  if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) {
    perror("setsockopt");
    freeaddrinfo(res);
    close(sockfd);
    return (-1);
  }

  if (-1 == bind(sockfd, res->ai_addr, res->ai_addrlen)) {
    perror("binding socket");
    freeaddrinfo(res);
    close(sockfd);
    return (-1);
  }
  printf("bind OK\n");

  if (-1 == listen(sockfd, BACKLOG)) {
    perror("socket listen");
    freeaddrinfo(res);
    close(sockfd);
    return (-1);
  }
  printf("listen OK\n");

  // can now accept incoming connections
  //
  struct sockaddr_storage inc_addr;
  socklen_t inc_addr_size = sizeof inc_addr;
  clientfd = accept(sockfd, (struct sockaddr *)&inc_addr, &inc_addr_size);
  if (-1 == clientfd) {
    perror("client accept");
    shutdown(sockfd, SHUT_RDWR);
    freeaddrinfo(res);
  }
  printf("client accept OK\n");

  send(clientfd, b.ssd.framebuf->framebuf, sizeof(uint8_t) * 1024, 0);
  uic_segment16_attr_t init = {
      .color = true,
      .dot = false,
      .height = 32,
      .origin =
          {
              .x = 10,
              .y = 10,
          },
  };

  ssd1306_fb_clear_buffer(b.ssd.framebuf, false);
  for (int i = 0; i <= 10; i++) {
    uic_t *seg = uic_segment16_new_from_int(i, &init, 4);
    seg->draw(b.ssd.framebuf, seg->attr);
    free(seg);
    ssd1306_write_framebuffer_all(b.ssd);
    send(clientfd, b.ssd.framebuf->framebuf, sizeof(uint8_t) * 1024, 0);
    ssd1306_fb_clear_buffer(b.ssd.framebuf, false);
  }
  printf("sent the buffer\n");

  bonnet_set_display_off(b);
  bonnet_close(&b);

  shutdown(sockfd, SHUT_RDWR);

  return 0;
}

int send_framebuffer_data(int clientfd, struct bonnet b) {
  // get the framebuffer size to know how much information needs to be
  // send to the client

  int buf_len = b.ssd.framebuf->height * b.ssd.framebuf->width;
  buf_len /= 8; // divide by the size of a byte, as height and width are the
                // pixel count

  return send(clientfd, b.ssd.framebuf->framebuf, buf_len, 0);
}
