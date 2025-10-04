#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "./lib/adafruit-oled-bonnet/src/bonnet.h"
#include "./lib/adafruit-oled-bonnet/src/ssd1306.h"
#include "./lib/adafruit-oled-bonnet/src/ssd1306_gl.h"
#include "./lib/adafruit-oled-bonnet/src/uic/cursor.h"
#include "./lib/adafruit-oled-bonnet/src/uic/segment16.h"

#define PORT "8080"
#define BACKLOG 10

void *socket_connect_handler(void *data);

enum prog_state {
  PROG_STATE_START,
  PROG_STATE_PROG_RUN,
};

enum conn_state {
  CONN_STATE_CONNECTED,
  CONN_STATE_DISCONNECTED,
  CONN_STATE_NOT_CONNECTED,
  CONN_STATE_ERR,
};

typedef struct game_master {
  enum prog_state prog_state;
  enum conn_state conn_state;
  pthread_mutex_t conn_mutex;
  int sockfd;
  int clientfd;
} game_master_t;

void game_master_init(game_master_t *gm) {
  gm->conn_state = CONN_STATE_NOT_CONNECTED;
  gm->prog_state = PROG_STATE_START;
  pthread_mutex_init(&gm->conn_mutex, NULL);
  gm->sockfd = 0;
}

bool is_button_exit_condition(bonnet_e_button_state a,
                              bonnet_e_button_state b) {
  return a == bonnet_e_button_state_down && b == bonnet_e_button_state_down;
}

int main(void) {

  game_master_t gm;
  game_master_init(&gm);

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
      .y = 8,
  };

  ssd1306_fb_vec2_t ip_origin = {
      .x = 1,
      .y = 18,
  };

  char *intro_str = "Connect to:";
  ssd1306_fb_draw_8x8font_str(b.ssd.framebuf, intro_origin, intro_str,
                              strlen(intro_str), true, true);
  ssd1306_fb_draw_8x8font_str(b.ssd.framebuf, ip_origin, ip, strlen(ip), true,
                              true);

  ssd1306_fb_vec2_t connect_a_loc = {
      .x = 10,
      .y = 44,
  };
  ssd1306_fb_vec2_t skip_connect_loc = {
      .x = 24,
      .y = 44,
  };

  ssd1306_fb_draw_circle(b.ssd.framebuf, 12, 48, 8, true, false);
  ssd1306_fb_draw_8x8font_str(b.ssd.framebuf, connect_a_loc, "A", 1, true,
                              true);
  char *skip_conn = "Skip connect";
  ssd1306_fb_draw_8x8font_str(b.ssd.framebuf, skip_connect_loc, skip_conn,
                              strlen(skip_conn), true, true);
  ssd1306_write_framebuffer_all(b.ssd);

  pthread_t connect_thread;
  pthread_create(&connect_thread, NULL, socket_connect_handler, (void *)&gm);

  uic_cursor_attr_t cursor_attr = {.origin = {
                                       .x = 10,
                                       .y = 10,
                                   }};

  uic_t *cursor = uic_cursor_new(&cursor_attr);
  bonnet_e_button_state states[7];
  // pressing A and B quits the program
  bonnet_button_get_states(b, states);
  while (!is_button_exit_condition(states[BONNET_BUTTON_IDX_A],
                                   states[BONNET_BUTTON_IDX_B])) {

    bonnet_button_get_states(b, states);

    if (gm.prog_state == PROG_STATE_START) {
      if ((states[BONNET_BUTTON_IDX_A] == bonnet_e_button_state_down &&
           gm.prog_state == PROG_STATE_START) ||
          (gm.prog_state == PROG_STATE_START &&
           gm.conn_state == CONN_STATE_CONNECTED)) {
        gm.prog_state = PROG_STATE_PROG_RUN;
      }
    }

    if (gm.prog_state == PROG_STATE_PROG_RUN) {
      ssd1306_fb_clear_buffer(b.ssd.framebuf, false);

      if (states[BONNET_BUTTON_IDX_RIGHT] == bonnet_e_button_state_down) {
        uic_cursor_position_update_relative(cursor, 10, 0);
      }
      if (states[BONNET_BUTTON_IDX_LEFT] == bonnet_e_button_state_down) {
        uic_cursor_position_update_relative(cursor, -10, 0);
      }
      if (states[BONNET_BUTTON_IDX_UP] == bonnet_e_button_state_down) {
        uic_cursor_position_update_relative(cursor, 0, -10);
      }
      if (states[BONNET_BUTTON_IDX_DOWN] == bonnet_e_button_state_down) {
        uic_cursor_position_update_relative(cursor, 0, 10);
      }

      cursor->draw(b.ssd.framebuf, cursor->attr);
      ssd1306_write_framebuffer_all(b.ssd);
      if (gm.conn_state == CONN_STATE_CONNECTED) {
        send(gm.clientfd, b.ssd.framebuf->framebuf, sizeof(uint8_t) * 1024, 0);
      }
    }
    // for (int i = 0; i < 7; i++) {
    //   printf("btn %d state %d\n", i, states[i]);
    // }
  }
  printf("game loop left\n");
  sleep(1);
  bonnet_set_display_off(b);
  bonnet_close(&b);
  pthread_join(connect_thread, NULL);

  shutdown(gm.sockfd, SHUT_RDWR);

  return 0;
}

void *socket_connect_handler(void *_gm) {

  game_master_t *gm = (game_master_t *)_gm;
  struct addrinfo hints;
  struct addrinfo *res;
  int status;

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
    gm->conn_state = CONN_STATE_ERR;
    return NULL;
  }

  gm->sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (-1 == gm->sockfd) {
    freeaddrinfo(res);
    perror("opening socket");
    gm->conn_state = CONN_STATE_ERR;
    return NULL;
  }
  printf("gm->sockfd creation OK\n");

  // get rid of "Address already in use" error message
  //
  int yes = 1;
  if (-1 ==
      setsockopt(gm->sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) {
    perror("setsockopt");
    freeaddrinfo(res);
    close(gm->sockfd);
    gm->conn_state = CONN_STATE_ERR;
    return NULL;
  }

  if (-1 == bind(gm->sockfd, res->ai_addr, res->ai_addrlen)) {
    perror("binding socket");
    freeaddrinfo(res);
    close(gm->sockfd);
    gm->conn_state = CONN_STATE_ERR;
    return NULL;
  }
  printf("bind OK\n");

  if (-1 == listen(gm->sockfd, BACKLOG)) {
    perror("socket listen");
    freeaddrinfo(res);
    close(gm->sockfd);
    gm->conn_state = CONN_STATE_ERR;
    return NULL;
  }
  printf("listen OK\n");

  // can now accept incoming connections
  //
  struct sockaddr_storage inc_addr;
  socklen_t inc_addr_size = sizeof inc_addr;
  gm->clientfd =
      accept(gm->sockfd, (struct sockaddr *)&inc_addr, &inc_addr_size);
  if (-1 == gm->clientfd) {
    perror("client accept");
    shutdown(gm->sockfd, SHUT_RDWR);
    freeaddrinfo(res);
  }
  gm->conn_state = CONN_STATE_CONNECTED;
  printf("client accept OK\n");

  return NULL;
}
