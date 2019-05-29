#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <libvepseudo.h>

uint64_t func(veos_handle *handle, uint64_t vemva)
{
  uint64_t recv_msg;
  uint64_t send_msg = 0xcafebaadbeefdead;
  int ret = ve_recv_data(handle, vemva, sizeof(uint64_t), &recv_msg);
  if (!ret) {
    printf("[VH] Success receiving msg! %lx\n", recv_msg);
  } else {
    perror("[VH] Fail to receive msg!");
    return ret;
  }
  ret = ve_send_data(handle, vemva, sizeof(uint64_t), &send_msg);
  if (!ret) {
    printf("[VH] Success sending msg!   %lx\n", send_msg);
  } else {
    perror("[VH] Fail to send msg!");
    return ret;
  }
  return 0;
}
