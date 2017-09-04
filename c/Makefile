# minimalist makefile
######################
# To add a competitive technique, simply add your code in the src subdirectory (follow the README.md instructions) and
# add your executable file name to the EXECUTABLES variable below.
# along with a target to build it.
#######################
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h

.PHONY: clean
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
OSFLAGS= -Wl,--no-as-needed
endif

ifeq ($(DEBUG),1)
CFLAGS = -fuse-ld=gold -fPIC  -std=c99 -ggdb -mavx2 -mbmi2 -march=native -Wall -Wextra -Wshadow -fsanitize=undefined  -fno-omit-frame-pointer -fsanitize=address  $(OSFLAGS) -ldl
CXXFLAGS = -fuse-ld=gold -fPIC  -std=c++11 -ggdb -mavx2 -mbmi2 -march=native -Wall -Wextra -Wshadow -fsanitize=undefined  -fno-omit-frame-pointer -fsanitize=address -Wno-deprecated-register  $(OSFLAGS) -ldl
ROARFLAGS = -DCMAKE_BUILD_TYPE=Debug -DSANITIZE=ON
else
CFLAGS = -fPIC -std=c99 -O3 -mavx2 -mbmi2 -march=native -Wall -Wextra -Wshadow   $(OSFLAGS) -ldl
CXXFLAGS = -fPIC -std=c++11 -O3 -mavx2 -mbmi2  -march=native -Wall -Wextra -Wshadow -Wno-deprecated-register  $(OSFLAGS) -ldl
ROARFLAGS = -DCMAKE_BUILD_TYPE=Release
endif # debug



EXECUTABLES=roaring_benchmarks ewah32_benchmarks 

all: $(EXECUTABLES)

test:
	./scripts/all.sh



src/roaring.c :
	(cd src && exec ../CRoaring/amalgamation.sh && rm almagamation_demo.c && rm almagamation_demo.cpp)


roaring_benchmarks : src/roaring.c src/roaring_benchmarks.c
	$(CC) $(CFLAGS) -o roaring_benchmarks src/roaring_benchmarks.c


ewah32_benchmarks: src/ewah32_benchmarks.cpp
	$(CXX) $(CXXFLAGS)  -o ewah32_benchmarks ./src/ewah32_benchmarks.cpp -IEWAHBoolArray/headers

clean:
	rm -r -f   $(EXECUTABLES) src/roaring.c src/roaring.h src/roaring.hh 
