# Edit this file to define constants and custom build targets.
# Please refer to the makemake documentation for more information.
#
# To compile multiple versions of a program or library, please study
#     http://www.its.caltech.edu/~jafl/jcc/multi_version.html

# Useful directories

MYCODEDIR := .

# Directories to search for header files

SEARCHDIRS := -iquote -I${MYCODEDIR} -I${MYCODEDIR}/jama/ -I/usr/local/include -I/usr/include

# makemake variables

# DEPENDFLAGS := -g -Wall -Werror ${SEARCHDIRS}
# following is for debugging (-g)
#DEPENDFLAGS := -g -Wall ${SEARCHDIRS}
#DEPENDFLAGS := -Wall ${SEARCHDIRS} -O3 -m486 -mcpu=k6 -march=k6
# Optimizations for Via Nemiah (c3-2)
#DEPENDFLAGS := -Wall ${SEARCHDIRS} -march=c3-2 -msse -mmmx -mfpmath=sse -O3 -pipe -fomit-frame-pointer 
#DEPENDFLAGS := -Wall ${SEARCHDIRS} -O3 -march=athlon-xp -msse -mmmx -mfpmath=sse -pipe -fomit-frame-pointer
DEPENDFLAGS := -Wall ${SEARCHDIRS} -O3 -ffast-math

# C preprocessor (C, C++, FORTRAN)
CPPFLAGS = 

# C compiler
CC     := gcc
CFLAGS  = ${DEPENDFLAGS}

%.o : %.c
	${CC} ${CPPFLAGS} ${CFLAGS} -c $< -o $@

# C++ compiler
CXX      := g++
CXXFLAGS  = ${DEPENDFLAGS}

%.o : %.cc
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c $< -o $@

%.o : %.C
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c $< -o $@

%.o : %.cpp
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c $< -o $@

%.o : %.cxx
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c $< -o $@

# C/C++/Eiffel/FORTRAN linker
LINKER    := gcc
LDFLAGS    = 
#LOADLIBES := -L /usr/lib -lstdc++ -L ${MYCODEDIR} -lsrfftw -lsfftw
#LOADLIBES := -L /usr/lib -lstdc++ -L ${MYCODEDIR} -lfftw3f
# To statically link the FFT libs:
#LOADLIBES := -static-libgcc ./libstdc++.a -lm -L ${MYCODEDIR} /usr/lib/libfftw3f.a
LOADLIBES := -L/usr/lib -L/usr/local/lib -lstdc++ -lm -lfftw3f

# This is what makemake added

# runVischeck3

runVischeck3 : ./colorTools.o ./imglib.o ./runSimulation.o ./kernlib.o ./main.o
	${LINKER} ${LDFLAGS} -o $@ ${filter-out %.a %.so, $^} ${LOADLIBES}

# target for making everything

.PHONY : all
all: runVischeck3


# target for removing all object files

.PHONY : tidy
tidy::
	@${RM} core ./colorTools.o ./imglib.o ./kernlib.o ./main.o ./runSimulation.o

# target for removing all object files

.PHONY : clean
clean:: tidy
	@${RM} runVischeck3

# list of all source files

MM_ALL_SOURCES := ./colorTools.cxx ./imglib.cxx ./kernlib.cxx ./main.cxx ./runSimulation.cxx


# target for checking a source file

CHECKSYNTAXFILE := ${basename ${filter %${CHECKSTRING}, ${MM_ALL_SOURCES}}}

.PHONY : checksyntax
checksyntax:
  ifneq (${CHECKSYNTAXFILE},)
	@${MAKE} ${addsuffix .o, ${CHECKSYNTAXFILE}}
  else
	@echo No target to make ${CHECKSTRING}
  endif


# target for touching appropriate source files

.PHONY : touch
touch::
	@list=$$(grep -l ${TOUCHSTRING} ${MM_ALL_SOURCES}); \
     for file in $$list; do { echo $$file; touch $$file; } done


# target for calculating dependencies (MAKEMAKE)

.PHONY : jdepend
jdepend:
	@${MAKEMAKE} --depend Makefile -- ${DEPENDFLAGS} --  ./colorTools.cxx ./colorTools.o ./imglib.cxx ./imglib.o ./kernlib.cxx ./kernlib.o ./main.cxx ./main.o ./runSimulation.cxx ./runSimulation.o


# DO NOT DELETE THIS LINE -- makemake depends on it.

./colorTools.o: ./colorTools.h /usr/include/stdio.h /usr/include/stdlib.h

./imglib.o: ./imglib.h ./kernlib.h /usr/include/math.h /usr/include/stdlib.h

./kernlib.o: ./imglib.h ./kernlib.h /usr/include/math.h /usr/include/stdlib.h

./main.o: ./runSimulation.h /usr/include/stdio.h /usr/include/stdlib.h /usr/include/time.h

./runSimulation.o: ./colorTools.h ./imglib.h ./kernlib.h ./runSimulation.h /usr/include/math.h /usr/include/time.h

