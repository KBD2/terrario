#ifndef __PROFILE_H__
#define __PROFILE_H__

#define CHAR_MAX 64
#define MARK_MAX 64

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct mark_t mark_t;
typedef struct profile_t profile_t;

struct mark_t {
  char name[CHAR_MAX];
  uint64_t length;
};

struct profile_t {
  char name[CHAR_MAX];

  mark_t marks[MARK_MAX];
  int mark_cnt;

  uint64_t curr;
};

#ifndef PROFILE_MODE

#define profile_time()
#define profile_start(pfl, str)
#define profile_mark(pfl, str)
#define profile_end(pfl)

#else

uint64_t profile_time(void) {
  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &time);
  return time.tv_sec * 1000000000 + time.tv_nsec;
}

void profile_start(profile_t *profile, const char *name) {
  profile->curr = profile_time();
  profile->mark_cnt = 0;

  memset(profile->name, 0, CHAR_MAX);
  strncpy(profile->name, name, CHAR_MAX - 1);
}

void profile_mark(profile_t *profile, const char *name) {
  clock_t curr = profile_time();

  for (int i = 0; i < profile->mark_cnt; i++) {
    if (!strcmp(profile->marks[i].name, name)) {
      profile->marks[profile->mark_cnt].length += curr - profile->curr;

      profile->curr = profile_time();
      return;
    }
  }

  if (profile->mark_cnt >= MARK_MAX) return;

  profile->marks[profile->mark_cnt].length = curr - profile->curr;

  memset(profile->marks[profile->mark_cnt].name, 0, CHAR_MAX);
  strncpy(profile->marks[profile->mark_cnt].name, name, CHAR_MAX - 1);

  profile->mark_cnt++;

  profile->curr = profile_time();
}

void profile_end(profile_t *profile) {
  if (!profile->mark_cnt) return;

  long double total = 0;

  for (int i = 0; i < profile->mark_cnt; i++)
    total += profile->marks[i].length;

  printf("[PROFILE] Profiler results(%s):\n", profile->name);

  for (int i = 0; i < profile->mark_cnt; i++) {
    long double percent = (100.0 * profile->marks[i].length) / total;
    printf("          \"%s\": %.2Lf%% / %.3f ms\n", profile->marks[i].name, percent, profile->marks[i].length / 1000000.0);
  }


  printf("[PROFILE] Total time: %.3Lf milliseconds.\n\n", total / 1000000.0);
}

#endif
#endif
