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


# input directories with C++ sources (must be *.cc extension) and headers
SRCDIR	 = ./src
INCDIR	 = ./include

# output directory with object code and binaries
BUILDDIR = ../../../build/dpseg

# C++ compiler to use and option passed during compilation and link
# (DNDEBUG flag disable some debug related code)
CXXFLAGS = -Wall -O3 -std=c++11 -DNDEBUG
LDLIB	 = -lboost_program_options

# source files must be *.cc files in $(SRCDIR)
SOURCES	 = Data Estimators Scoring Sentence
OBJECTS	 = $(SOURCES:%=%.o)


.PHONY: all clean

all: dpseg

clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/dpseg

%.o: $(SRCDIR)/%.cc
	$(CXX) -o $(BUILDDIR)/$@ -c $^ $(CXXFLAGS) -I$(INCDIR)

dpseg: $(OBJECTS) dpseg.o
	$(CXX) -o $(BUILDDIR)/$@ $(^:%=$(BUILDDIR)/%) $(LDLIB)
