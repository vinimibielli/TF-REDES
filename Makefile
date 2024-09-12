TARGET = CROSS_WORD.bin

SRCDIR = srcs
INCLUDEDIR = $(SRCDIR)/include

SOURCE = $(wildcard $(SRCDIR)/*.cpp)
HEADERS = $(wildcard $(INCLUDEDIR)/*.h)

CC = g++

all: $(TARGET)

$(TARGET): $(SOURCE) $(HEADERS)
	$(CC) -o $(TARGET) $(SOURCE) -I$(INCLUDEDIR)