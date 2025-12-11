# Contrail Manager Makefile
# Uses ESMF library, so requires ESMFMKFILE path be set in environment

ifneq ($(origin ESMFMKFILE), environment)
$(error Environment variable ESMFMKFILE was not set.)
endif

include $(ESMFMKFILE)

# Define static library name
TARGET = libcontrailmanager.a

################################################################################
################################################################################

.DEFAULT_GOAL := $(TARGET)

INCLUDE = include

OBJDIR = obj
DEPDIR = dep

SRCS = $(wildcard *.cpp)
OBJS = $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))
DEPS = $(addprefix $(DEPDIR)/, $(SRCS:.cpp=.d))

-include $(DEPS)

$(OBJDIR)/%.o : %.cpp
	@mkdir -p $(OBJDIR) $(DEPDIR)
	$(ESMF_CXXCOMPILER) -c $(ESMF_CXXCOMPILEOPTS) -MMD -MP -MF $(DEPDIR)/$*.d -MT $(OBJDIR)/$*.o \
	$(ESMF_CXXCOMPILEPATHSLOCAL) $(ESMF_CXXCOMPILEPATHS) $(ESMF_CXXCOMPILECPPFLAGS) -I$(INCLUDE) $< -o $@

$(TARGET): $(OBJS)
	@echo "--- Archiving object files into $(TARGET) ---"
	ar rcs $@ $^

# -----------------------------------------------------------------------------
.PHONY: clean

clean:
	rm -rf $(TARGET) $(OBJDIR) $(DEPDIR)