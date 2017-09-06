#define __STDC_FORMAT_MACROS 1
#define _GNU_SOURCE

#ifdef RECORD_MALLOCS
#include "cmemcounter.h"
#endif

#include "numbersfromtextfiles.h"
#include "roaring.c"
#include <inttypes.h>
#include <stdio.h>
#include <sys/time.h>

bool roaring_iterator_increment(uint32_t value, void *param) {
  size_t count;
  memcpy(&count, param, sizeof(uint64_t));
  count++;
  memcpy(param, &count, sizeof(uint64_t));
  (void)value;
  return true; // continue till the end
}

/**
 * Once you have collected all the integers, build the bitmaps.
 */
static roaring_bitmap_t **create_all_bitmaps(size_t *howmany,
                                             uint32_t **numbers, size_t count,
                                             bool runoptimize, bool copyonwrite,
                                             bool verbose,
                                             uint64_t *totalsize) {
  *totalsize = 0;
  if (numbers == NULL)
    return NULL;
  size_t savedmem = 0;
  roaring_bitmap_t **answer = malloc(sizeof(roaring_bitmap_t *) * count);
  for (size_t i = 0; i < count; i++) {
    answer[i] = roaring_bitmap_of_ptr(howmany[i], numbers[i]);
    answer[i]->copy_on_write = copyonwrite;
    if (runoptimize)
      roaring_bitmap_run_optimize(answer[i]);
    savedmem += roaring_bitmap_shrink_to_fit(answer[i]);
    *totalsize += roaring_bitmap_portable_size_in_bytes(answer[i]);
  }
  if (verbose)
    printf("saved bytes by shrinking : %zu \n", savedmem);
  return answer;
}

static void printusage(char *command) {
  printf(" Try %s directory \n where directory could be "
         "benchmarks/realdata/census1881\n",
         command);
  ;
  printf("the -r flag turns on run optimization");
}

int main(int argc, char **argv) {
  int c;
  bool runoptimize = true;
  bool verbose = false;
  uint64_t successive_and = 0;
  uint64_t successive_or = 0;
  uint64_t total_or = 0;

  bool copyonwrite = false;
  char *extension = ".txt";
  while ((c = getopt(argc, argv, "cvre:h")) != -1)
    switch (c) {
    case 'e':
      extension = optarg;
      break;
    case 'v':
      verbose = true;
      break;
    case 'r':
      runoptimize = true;
      if (verbose)
        printf("enabling run optimization\n");
      break;
    case 'c':
      copyonwrite = true;
      if (verbose)
        printf("enabling copyonwrite\n");
      break;
    case 'h':
      printusage(argv[0]);
      return 0;
    default:
      abort();
    }
  if (optind >= argc) {
    printusage(argv[0]);
    return -1;
  }
  char *dirname = argv[optind];
  size_t count;

  size_t *howmany = NULL;
  uint32_t **numbers =
      read_all_integer_files(dirname, extension, &howmany, &count);
  if (numbers == NULL) {
    printf("I could not find or load any data file with extension %s in "
           "directory %s.\n",
           extension, dirname);
    return -1;
  }
  uint32_t maxvalue = 0;
  for (size_t i = 0; i < count; i++) {
    if (howmany[i] > 0) {
      if (maxvalue < numbers[i][howmany[i] - 1]) {
        maxvalue = numbers[i][howmany[i] - 1];
      }
    }
  }
  uint64_t totalcard = 0;
  for (size_t i = 0; i < count; i++) {
    totalcard += howmany[i];
  }
  uint64_t successivecard = 0;
  for (size_t i = 1; i < count; i++) {
    successivecard += howmany[i - 1] + howmany[i];
  }

  uint64_t totalsize = 0;
  roaring_bitmap_t **bitmaps = create_all_bitmaps(
      howmany, numbers, count, runoptimize, copyonwrite, verbose, &totalsize);
  if (bitmaps == NULL)
    return -1;
  int warmup = 5;

  for (int w = 0; w < warmup; w++) {
    for (int i = 0; i < (int)count - 1; ++i) {
      roaring_bitmap_t *tempand =
          roaring_bitmap_and(bitmaps[i], bitmaps[i + 1]);
      successive_and += roaring_bitmap_get_cardinality(tempand);
      roaring_bitmap_free(tempand);
    }
  }

  struct timeval st, et;
  int elapsed;
  gettimeofday(&st, NULL);
  for (int i = 0; i < (int)count - 1; ++i) {
    roaring_bitmap_t *tempand = roaring_bitmap_and(bitmaps[i], bitmaps[i + 1]);
    successive_and += roaring_bitmap_get_cardinality(tempand);
    roaring_bitmap_free(tempand);
  }

  gettimeofday(&et, NULL);
  elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);

  printf("Successive intersections took %d mus\n", elapsed);

  for (int w = 0; w < warmup; w++) {
    for (int i = 0; i < (int)count - 1; ++i) {
      roaring_bitmap_t *tempor = roaring_bitmap_or(bitmaps[i], bitmaps[i + 1]);
      successive_or += roaring_bitmap_get_cardinality(tempor);
      roaring_bitmap_free(tempor);
    }
  }

  gettimeofday(&st, NULL);
  for (int i = 0; i < (int)count - 1; ++i) {
    roaring_bitmap_t *tempor = roaring_bitmap_or(bitmaps[i], bitmaps[i + 1]);
    successive_or += roaring_bitmap_get_cardinality(tempor);
    roaring_bitmap_free(tempor);
  }
  gettimeofday(&et, NULL);
  elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);

  printf("Successive unions took %d mus\n", elapsed);
  for (int w = 0; w < warmup; w++) {
    roaring_bitmap_t *totalorbitmap =
        roaring_bitmap_or_many(count, (const roaring_bitmap_t **)bitmaps);
    total_or = roaring_bitmap_get_cardinality(totalorbitmap);
    roaring_bitmap_free(totalorbitmap);
  }

  gettimeofday(&st, NULL);
  roaring_bitmap_t *totalorbitmap =
      roaring_bitmap_or_many(count, (const roaring_bitmap_t **)bitmaps);
  total_or = roaring_bitmap_get_cardinality(totalorbitmap);
  roaring_bitmap_free(totalorbitmap);
  gettimeofday(&et, NULL);
  elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
  printf("Total unions took %d mus\n", elapsed);

  for (int i = 0; i < (int)count; ++i) {
    free(numbers[i]);
    numbers[i] = NULL; // paranoid
    roaring_bitmap_free(bitmaps[i]);
    bitmaps[i] = NULL; // paranoid
  }
  free(bitmaps);
  free(howmany);
  free(numbers);

  return 0;
}
