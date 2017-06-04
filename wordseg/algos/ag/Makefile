# Copyright 2009 - 2014 Mark Johnson
# Copyright 2017 Mathieu Bernard
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.



# The following variables are defined in the setup.py: BUILDDIR is the
# output directory with object code and binaries
#
BUILDDIR = ./build


# directory with C++ sources (must be *.cc extension) and headers
SRCDIR  = ./src
INCDIR  = ./include

# some flags (yes or no)
DEBUG   = no  # compile for debug
QUAD    = no  # quad precision (float128, slow but avoid underflows)
MP	= no  # OpenMP multi-threaded computation

# TODO fix that... (use cmake? good idea for boost as well)
# define CXXFLAGS and LDFLAGS according to the above flags
ifeq ($(DEBUG),yes)
  CXXFLAGS = -Wall -g -I$(INCDIR) -DBOOST_LOG_DYN_LINK
else
  CXXFLAGS = -Wall -O3 -std=c++11 -I$(INCDIR) -DBOOST_LOG_DYN_LINK
  LDFLAGS  = -g
endif

ifeq ($(QUAD),yes)
  QUADFLAG =  -DQUADPREC
endif

ifeq ($(MP),yes)
  MPFLAG = -fopenmp
endif


# source files must be *.cpp files in $(SRCDIR)
SOURCES	 = gammadist mt19937ar sym logging
OBJECTS	 = $(SOURCES:%=$(BUILDDIR)/%.o)

LDLIB	 = -pthread -lboost_program_options -lboost_date_time \
		-lboost_filesystem -lboost_system -lboost_log -lboost_thread


.PHONY: all clean  # run-toy

all: pycfg

clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/pycfg


$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -o $@ -c $? $(CXXFLAGS) -I$(INCDIR) $(MPFLAG) $(QUADFLAG)

pycfg: $(OBJECTS) $(BUILDDIR)/pycfg.o
	$(CXX) -o $(BUILDDIR)/$@ $^ $(LDLIB) $(MPFLAG)

# pycfg-quad: gammadist.o pycfg-quad.o mt19937ar.o sym.o
# 	$(CXX) $^ -lm $(LDFLAGS) -o $@

# pycfg-mp:  gammadist.o pycfg-mp.o mt19937ar.o sym.o
# 	$(CXX) -fopenmp $^ -lm $(LDFLAGS) -o $@

# pycfg-quad-mp: gammadist.o pycfg-quad-mp.o mt19937ar.o sym.o
# 	$(CXX) -fopenmp $^ -lm $(LDFLAGS) -o $@

# pycfg-quad.o: $(SRCDIR)/pycfg.cc
# 	$(CXX) -c $(CXXFLAGS) -DQUADPREC $< -o $@

# pycfg-mp.o: $(SRCDIR)/pycfg.cc
# 	$(CXX) -c $(CXXFLAGS) -fopenmp $^ -o $@

# pycfg-quad-mp.o: $(SRCDIR)/pycfg.cc
# 	$(CXX) -c $(CXXFLAGS) -fopenmp -DQUADPREC $< -o $@

# gammadist.o: $(SRCDIR)/gammadist.cc
# 	$(CXX) -c $(CXXFLAGS) $< -o $@

# mt19937ar.o: $(SRCDIR)/mt19937ar.cc
# 	$(CXX) -c $(CXXFLAGS) $< -o $@


# run-toy: pycfg pycfg-quad testengger.lt testeng.yld
# 	$(EXEC) pycfg -r 1234 -P -D -R -1 -d 100 -a 1e-2 -b 1 -e 1 -f 1 \
# 		-g 1e2 -h 1e-2 -n 10 -C -E -A testeng.prs -N 10 \
# 		-F testeng.trace -G testeng.wlt testengger.lt \
# 		-X "cat > testeng.X1.prs" -X "cat > testeng.X2.prs" \
# 		-u testeng1.yld -U "cat > testeng1.prs" -v testeng2.yld \
# 		-V "cat > testeng2.prs" < testeng.yld
# 	$(EXEC) pycfg-quad -r 1234 -P -D -R -1 -d 100 -a 1e-2 -b 1 -e 1 -f 1 \
# 		-g 1e2 -h 1e-2 -n 10 -C -E -A q-testeng.prs -N 10 \
# 		-F q-testeng.trace -G q-testeng.wlt testengger.lt \
# 		-X "cat > q-testeng.X1.prs" -X "cat > q-testeng.X2.prs" \
# 		-u testeng1.yld -U "cat > q-testeng1.prs" -v testeng2.yld \
# 		-V "cat > q-testeng2.prs" < testeng.yld
