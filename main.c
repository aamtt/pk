#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

static off_t start_of_last_lines(int fd, long n) {
  off_t end = lseek(fd, 0, SEEK_END);
  if (n <= 0)
      return end;

  off_t pos = end;
  if (end > 0) {
    char last;
    if (pread(fd, &last, 1, end - 1) == 1 && last == '\n')
        pos = end - 1;
  }

  char buf[8192];
  long seen = 0;
  while (pos > 0) {
    size_t want = pos < (off_t)sizeof buf ? (size_t)pos : sizeof buf;
    pos -= want;
    if (pread(fd, buf, want, pos) != (ssize_t)want)
      return 0;
    for (size_t i = want; i > 0; i--) {
      if (buf[i - 1] == '\n' && ++seen == n)
          return pos + i;
    }
  }
  return 0;
}

static int write_all(const char *buf, size_t len) {
  while (len > 0) {
    ssize_t w = write(STDOUT_FILENO, buf, len);
    if (w < 0)
      return -1;
    buf += w;
    len -= w;
  }
  return 0;
}


static int term_width(void) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
    return ws.ws_col;
  return 80; // fallback
}

static void reserve(long rows) {
  for (long i = 0; i < rows; i++)
    putchar('\n');
  printf("\033[%ldA\0337", rows);
  fflush(stdout);
}

static void redraw(char **ring, size_t *lens, long total, long n, int width) {
  long m = n + 1;
  long rows = total < n ? total : n;
  fputs("\0338", stdout);
  for (long i = total - rows; i < total; i++) {
    size_t len = lens[i % m];
    if (len > (size_t)width)
      len = width;
    printf("\r\033[2K%.*s", (int)len, ring[i % n]);
    if (i < total - 1)
      putchar('\n');
  }
  fflush(stdout);
}

static int tail_stream(int fd, long n) {
  char buf[65536];
  ssize_t got;

  if (n <= 0) {
    while (read(fd, buf, sizeof buf) > 0)
      ;
    return 0;
  }

  long m = n + 1;
  char **ring = calloc(m, sizeof *ring);
  size_t *lens = calloc(m, sizeof *lens);
  long count = 0;
  int live = isatty(STDOUT_FILENO);
  int width = term_width();

  if (live)
    reserve(n + 1);

  while ((got = read(fd, buf, sizeof buf)) > 0) {
    char *p = buf;
    char *end = buf + got;
    while (p < end) {
      char *nl = memchr(p, '\n', end - p);
      size_t seg = (nl ? nl : end) - p;
      long slot = count % m;
      ring[slot] = realloc(ring[slot], lens[slot] + seg + 1);
      memcpy(ring[slot] + lens[slot], p, seg);
      lens[slot] += seg;
      p += seg;
      if (nl) {
        p++;
        count++;
        lens[count % m] = 0;
      }
    }

    if (live)
      redraw(ring, lens, count + (lens[count % m] > 0), n, width);
  }

  long total = count + (lens[count % m] > 0);
  if (live) {
    if (total > 0)
      putchar('\n');
    return 0;
  }

  for (long i = total > n ? total - n : 0; i < total; i++) {
    fwrite(ring[i % m], 1, lens[i % m], stdout);
    putchar('\n');
  }
  return 0;
}

static int tail_follow(int fd, long n) {
  lseek(fd, start_of_last_lines(fd, n), SEEK_SET);

  char buf[65536];
  for (;;) {
    ssize_t got = read(fd, buf, sizeof buf);
    if (got < 0) {
      perror("pk");
      return 1;
    }

    if (got > 0) {
      if (write_all(buf, got) < 0)
        return 1;
      continue;
    }

    struct stat st;
    if (fstat(fd, &st) == 0 && st.st_size < lseek(fd, 0, SEEK_CUR))
      lseek(fd, 0, SEEK_SET);
    usleep(100000);  
  }
}

void help(void) {
    fprintf(stdout, "usage: pk [-n N] [FILE]\n\n");
    fprintf(stdout, "flags: \t[-h | --help] help command\n\t\t[-vV] version\n");
}

int main (int argc, char **argv) {
  long n = 10;
  const char *path = NULL;

  if (argc == 2 && strcmp(argv[1], "fire") == 0) {
    fprintf(stdout, "GAME!\n");
    return 0;
  }

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
        n = atol(argv[++i]);
    } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "-V") == 0) {
        fprintf(stdout, "v1.0.0\n");
        return 0;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
        help();
        return 0;
    } else {
        path = argv[i];
    }
  }

  if (!path) {
    if (isatty(STDIN_FILENO)) {
      fprintf(stderr, "usage: pk [-n N] [FILE]\n");
      return 2;
    }
    return tail_stream(STDIN_FILENO, n);
  }

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    perror(path);
    return 1;
  }

  return tail_follow(fd, n);  
}
