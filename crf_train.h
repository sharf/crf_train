#ifndef CRF_TRAIN_HEADER_H
#define CRF_TRAIN_HEADER_H
void init_objective(int n, NEWMAT::ColumnVector& x);
void objective(int mode, int n, const NEWMAT::ColumnVector& x, double& fx, NEWMAT::ColumnVector& g,
	   int& result);
#endif
