#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#include <stdio.h>
#include <sys/time.h>
#include "numbersfromtextfiles.h"
#ifdef __cplusplus
}
#endif

#include "ewah.h" /* EWAHBoolArray */

/**
 * Once you have collected all the integers, build the bitmaps.
 */
static std::vector<EWAHBoolArray<uint32_t> > create_all_bitmaps(size_t *howmany,
        uint32_t **numbers, size_t count) {
    if (numbers == NULL) return std::vector<EWAHBoolArray<uint32_t> >();
    std::vector<EWAHBoolArray<uint32_t> > answer(count);
    for (size_t i = 0; i < count; i++) {
        EWAHBoolArray<uint32_t> & bm = answer[i];
        uint32_t * mynumbers = numbers[i];
        for(size_t j = 0; j < howmany[i] ; ++j) {
            bm.set(mynumbers[j]);
        }
        bm.trim();
    }
    return answer;
}

static void printusage(char *command) {
    printf(
        " Try %s directory \n where directory could be "
        "benchmarks/realdata/census1881\n",
        command);
    ;
    printf("the -v flag turns on verbose mode");

}

int main(int argc, char **argv) {
    int c;
    uint64_t successive_and = 0;
    uint64_t successive_or = 0;
    uint64_t total_or = 0;

    const char *extension = ".txt";
    bool verbose = false;
    while ((c = getopt(argc, argv, "ve:h")) != -1) switch (c) {
        case 'e':
            extension = optarg;
            break;
        case 'v':
            verbose = true;
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
        printf(
            "I could not find or load any data file with extension %s in "
            "directory %s.\n",
            extension, dirname);
        return -1;
    }
    uint32_t maxvalue = 0;
    for (size_t i = 0; i < count; i++) {
      if( howmany[i] > 0 ) {
        if(maxvalue < numbers[i][howmany[i]-1]) {
           maxvalue = numbers[i][howmany[i]-1];
         }
      }
    }
    uint64_t totalcard = 0;
    for (size_t i = 0; i < count; i++) {
      totalcard += howmany[i];
    }
    uint64_t successivecard = 0;
    for (size_t i = 1; i < count; i++) {
       successivecard += howmany[i-1] + howmany[i];
    }

    std::vector<EWAHBoolArray<uint32_t> > bitmaps = create_all_bitmaps(howmany, numbers, count);
    if (bitmaps.empty()) return -1;
    uint64_t totalsize = 0;

    for (int i = 0; i < (int) count; ++i) {
        EWAHBoolArray<uint32_t> & bv = bitmaps[i];
        totalsize += bv.sizeInBytes(); // should be close enough to memory usage
    }
    struct timeval st, et;
    int elapsed;
    gettimeofday(&st,NULL);
    for (int i = 0; i < (int)count - 1; ++i) {
        EWAHBoolArray<uint32_t>  tempand;
        bitmaps[i].logicaland(bitmaps[i + 1],tempand);
        successive_and += tempand.numberOfOnes();
    }
    gettimeofday(&et,NULL);
    elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    printf("Successive intersections took %d mus\n", elapsed);

    gettimeofday(&st,NULL);
    for (int i = 0; i < (int)count - 1; ++i) {
        EWAHBoolArray<uint32_t>  tempor;
        bitmaps[i].logicalor(bitmaps[i + 1],tempor);
        successive_or += tempor.numberOfOnes();
    }
    gettimeofday(&et,NULL);
    elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);

    printf("Successive unions took %d mus\n", elapsed);

    gettimeofday(&st,NULL);
    if(count>1) {
        EWAHBoolArray<uint32_t>  totalorbitmap;
        bitmaps[0].logicalor(bitmaps[1],totalorbitmap);
        for(int i = 2 ; i < (int) count; ++i) {
          EWAHBoolArray<uint32_t>  tmp;
          totalorbitmap.logicalor(bitmaps[i],tmp);
          tmp.swap(totalorbitmap);
        }
        total_or = totalorbitmap.numberOfOnes();
    }
    gettimeofday(&et,NULL);
    elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    printf("Total unions took %d mus\n", elapsed);

    for (int i = 0; i < (int)count; ++i) {
        free(numbers[i]);
        numbers[i] = NULL;  // paranoid
    }
    free(howmany);
    free(numbers);

    return 0;
}
