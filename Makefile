CXX = g++
LD = g++
CFLAGS = -Wall -O3
LDFLAGS = -llzma -lpthread
BINDIR = bin
OBJDIR = obj

SRCS = $(wildcard *.cpp)
OBJS = $(addprefix $(OBJDIR)/, $(patsubst %.cpp,%.o,$(SRCS)))
RELEASE_OBJS := $(filter-out $(OBJDIR)/test.o,$(OBJS))
TEST_OBJS := $(filter-out $(OBJDIR)/colibri.o,$(OBJS))

colibri: $(RELEASE_OBJS)
	mkdir -p $(BINDIR)
	$(LD) $(LDFLAGS) -o $(BINDIR)/colibri $(RELEASE_OBJS)

test: $(TEST_OBJS)
	mkdir -p $(BINDIR)
	$(LD) $(LDFLAGS) -o $(BINDIR)/test $(TEST_OBJS)

$(OBJDIR)/%.o: %.cpp
	mkdir -p $(OBJDIR)
	$(LD) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -rf bin/ obj/
