# Contrail Manager Makefile
# Uses ESMF library, so requires ESMFMKFILE path be set in environment

ifneq ($(origin ESMFMKFILE), environment)
$(error Environment variable ESMFMKFILE was not set.)
endif

include $(ESMFMKFILE)

# Define static library name
TARGET=libcontrailmanager.a

################################################################################
################################################################################

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

%.o : %.cpp
	$(ESMF_CXXCOMPILER) -c $(ESMF_CXXCOMPILEOPTS) $(ESMF_CXXCOMPILEPATHSLOCAL) $(ESMF_CXXCOMPILEPATHS) $(ESMF_CXXCOMPILECPPFLAGS) $<

$(TARGET): $(OBJS)
	@echo "--- Archiving object files into $(TARGET) ---"
	ar rcs $@ $^

# -----------------------------------------------------------------------------
.PHONY: clean

clean:
	rm -f $(TARGET) $(OBJS)