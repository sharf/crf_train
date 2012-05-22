CC=g++
DEBUG=-g
LDFLAGS=-lblas -lfastlib -llapack -lm -lopt -lnewmat
CPPFLAGS=-DDISABLE_DISK_MATRIX -DHAVE_NAMESPACES -I$(FASTLIBPATH)/include -I$(MLPACK) $(DEBUG) 

crf: crf_train.o
		$(CC) -o crf crf_train.o $(DEBUG) $(LDFLAGS)

all:crf_train SparseVector_test

crf_train: crf_train.o $(MLPACK)/optimization/optimizers.o
		$(CC) -o crf_train crf_train.o $(DEBUG) $(MLPACK)/optimization/optimizers.o $(LDFLAGS)

SparseVector_test: SparseVector_test.o 
		$(CC) -o SparseVector_test SparseVector_test.o $(DEBUG)  $(LDFLAGS)
	
clean:
	rm -f crf_train SparseVector_test *.o
	rm -f tstLBFGS
