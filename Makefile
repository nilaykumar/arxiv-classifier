CC = g++
CFLAGS = -Wall
DEPS = arxiv-classifier.cpp
LIBS = libpugixml.a -lcurl

all: trainer bayes

trainer: trainer.cpp
	$(CC) $(CFLAGS) -o trainer.o trainer.cpp $(DEPS) $(LIBS)

bayes: bayes.cpp
	$(CC) $(CFLAGS) -o bayes.o bayes.cpp $(DEPS) $(LIBS)

.PHONY: clean
clean:
	rm -f *.o
