#include "SparseVector.h"

int main(int argc, char** argv){

	SparseVector<double> v;
	v.set(2,3);
	v.PrintDebug();
	v.set(4,5);
	v.set(2,1.4);
	v.PrintDebug();
}
