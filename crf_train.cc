

/** \example tstLBFGS.C
 *
 * Test program for LBFGS optimization objects
 * 
 * 1. Limited Memory BFGS method with More-Thuente Search on an NLF1
 *
 */

#ifdef HAVE_CONFIG_H
#include "OPT++_config.h"
#endif

#include <fstream>
#ifdef HAVE_STD
#include <cstdio>
#else
#include <stdio.h>
#endif

#include "OptLBFGS.h"
#include "NLF.h"

#define NUMBER_FILE_CHUNKS 18

using NEWMAT::ColumnVector;

using namespace OPTPP;
void update_model(int, int, ColumnVector) {}

void init_rosen(int n, NEWMAT::ColumnVector& x);
void rosen(int mode, int n, const NEWMAT::ColumnVector& x, double& fx, NEWMAT::ColumnVector& g,
	   int& result);

#include<fastlib/fastlib.h>
#include<cmath>
#include<iostream>
#include <iostream>
#include<fastlib/optimization/contrib/optimizers.h>
#include "NP_chunking_ser.h"
#include<cassert>
#include<sstream>


void LogMultiplyInit(const Matrix& log_A, const Matrix& log_B, Matrix* log_C);
double log_sum_exp(const Vector& logs);
Matrix* DotMultiplyInit(const Matrix* const A, const Matrix* const B, Matrix* C);
long double crf_objective(Vector& parameters, const Matrix& data, Vector* gradient) ;

void save_lambda(const Vector& lambda, const char* filename);


void save_featureset(Featureset& fs, const char* filename);
void load_featureset(Featureset& fs, const char* filename);

void PrintNonZeroDebug(const Vector& v);

long int num_features;
int num_labels;
Featureset fs;
const char* training_file, *parameter_file;


Vector lambda;
Vector gradient;


int main (int argc, char** argv)
{

	fx_module* root = fx_init(argc,argv,NULL);

	training_file = fx_param_str(root,"training_file","");
	parameter_file = fx_param_str(root,"parameter_file","");
	
	if(strcmp(training_file, "") != 0) {
		
		fs.setmode(TRAIN_MODE);
		std::cerr << "Training mode ... " << std::endl;
		std::cerr << "Initializing feature set class ..." << std::endl;
		fs.Init(training_file);

		//Init global constants
		num_features = fs.get_num_features();
		num_labels = fs.get_num_labels();
		std::cerr << "Number of features: " << num_features << std::endl;
		std::cerr << "Number of labels: " << num_labels << std::endl;


		//Init the parameter vector and its gradient
		lambda.Init(num_features);
		gradient.Init(num_features);

		//Init the optimizer

  int n = num_features;
  
  static char *status_file = {"tstLBFGS.out"};

  //  Create a Nonlinear problem object

  NLF1 nlp(n,rosen,init_rosen);
  
  //  Build a LBFGS object and optimize 

  OptLBFGS objfcn(&nlp);   
  objfcn.setUpdateModel(update_model);
  if (!objfcn.setOutputFile(status_file, 0))
    cerr << "main: output file open failed" << endl;
  objfcn.setGradTol(1.e-3);
  objfcn.setMaxBacktrackIter(10);
  objfcn.setPrintFinalX(false);
  objfcn.optimize();
    
  objfcn.printStatus("Solution from LBFGS: More and Thuente's linesearch");

#ifdef REG_TEST
  ColumnVector x_sol = nlp.getXc();
  double f_sol = nlp.getF();
  ostream* optout = objfcn.getOutputFile();
  if ((1.0 - x_sol(1) <= 1.e-2) && (1.0 - x_sol(2) <= 1.e-2) && (f_sol
								 <=
								 1.e-2))
    *optout << "LBFGS 1 PASSED" << endl;
  else
    *optout << "LBFGS 1 FAILED" << endl;
#endif

  objfcn.cleanup();



		std::cerr << "Finished optimizing. Training completed." << std::endl;

		save_lambda(lambda, parameter_file);	
		std::cerr << "Persisted lambda to file " << parameter_file << std::endl;

		save_featureset(fs, NULL);
		std::cerr << "Persisted featureset to file " << parameter_file << std::endl;
		return 0;

	}

	fx_done(root);

}


void init_rosen(int n, NEWMAT::ColumnVector& x) {

	for(int i = 1; i <= n; i++)
		x(i) = (double)rand()/RAND_MAX;
}

void rosen(int mode, int n, const NEWMAT::ColumnVector& x, double& fx, NEWMAT::ColumnVector& g,
	   int& result) {

	Matrix dummy;
	dummy.Init(0,0);
	for(int i = 0; i < n; i++)
		lambda[i] = x(i + 1);
	fx = crf_objective(lambda, dummy, &gradient);
	for(int i = 0; i < n; i++)
		g(i + 1) = gradient[i];
	result = mode;
}


long double crf_objective(Vector& lambda, const Matrix& data, Vector* gradient) {


	std::cerr << "start eval " << std::endl;
	//The length of a sample
	int n = 2;
	static int iter;

	double LL;
	LL = 0.0;
	//DEBUG
	double LL1_persist, LL2_persist;
	LL1_persist = LL2_persist = 0;
	//DEBUG
	gradient->SetZero();

	fs.go_to_begin(training_file);
	Vector X,T;
	X.Init(n);
	T.Init(n);
	X.SetZero();
	T.SetZero();

	int item = 0;
	
	while(fs.getX(X,T) != EOF){

		Vector& Y = fs.getY();

		double LL1, LL2;
		LL1 = LL2 = 0;


		//Note that M[i] is M_{i+1}
		Matrix M[n];
		Matrix log_M[n];

		//Compute {M_i}
		for(int i = 0; i < n; i++){
			M[i].Init(num_labels,num_labels);
			log_M[i].Init(num_labels,num_labels);

			for(int j = 0; j < num_labels; j++){
				for(int k = 0; k < num_labels; k++){
					M[i].set(k,j,exp(Dot(lambda,fs.get_feature_vector(k,j,X,T,i))));
					log_M[i].set(k,j,Dot(lambda,fs.get_feature_vector(k,j,X,T,i)));
				}
			}
		}

		
		//Special for i = 0 since  Y[i-1] will overflow for i = 0 
		LL1 += log_M[0].get(0,Y[0]);
		for(int i = 1; i < n; i++){
			LL1 += log_M[i].get(Y[i-1],Y[i]);
		}
		//Now compute the alpha and beta matrices to compute the the expectation. There are only n+1 of each so this is not an expensive computation

		//beta is actually beta_tranpose
		Matrix alpha[n+1], beta[n];
		Matrix log_alpha[n+1];
		Matrix log_beta[n];

		alpha[0].Init(1,num_labels);
		alpha[0].SetAll(1.0);

		log_alpha[0].Init(1,num_labels);
		log_alpha[0].SetZero();

		for(int i = 1; i <= n; i++){
			la::MulInit(alpha[i-1],M[i-1],&(alpha[i]));
			LogMultiplyInit(log_alpha[i-1],log_M[i-1],&(log_alpha[i]));
		}

		beta[n-1].Init(num_labels,1);
		beta[n-1].SetAll(1.0);

		log_beta[n-1].Init(num_labels,1);
		log_beta[n-1].SetZero();

		for(int i = n-2; i >= 0 ; i--){
			la::MulInit(M[i+1],beta[i+1],&(beta[i]));
			LogMultiplyInit(log_M[i+1],log_beta[i+1],&(log_beta[i]));
		}


		Matrix Z_dummy;
		Matrix tmp1;
		tmp1.Init(num_labels,1);
		tmp1.SetAll(1.0);
		la::MulInit(alpha[n],tmp1,&Z_dummy);

		//In log domain
		Matrix log_Z_dummy;
		Matrix log_tmp1;
		log_tmp1.Init(num_labels,1);
		log_tmp1.SetAll(0.0);
		LogMultiplyInit(log_alpha[n],log_tmp1,&log_Z_dummy);

		double Z = Z_dummy.get(0,0);
		assert(Z > 0);

		double log_Z = log_Z_dummy.get(0,0);
		LL2 = log_Z;


		LL1_persist += LL1;
		LL2_persist += LL2;
		LL += (LL1 - LL2);


		Vector F;
		F.Init(lambda.length());
		F.SetZero();
			
		AddTo(fs.get_feature_vector(0,Y[0],X,T,0),&F);
		for(int i = 1;i < n; i++)
			AddTo(fs.get_feature_vector(Y[i-1],Y[i],X,T,i),&F);

		Vector E_F;
		E_F.Init(lambda.length());
		E_F.SetZero();

	
		for(int i = 0; i < n; i++){
			for(int k = 0; k < num_labels; k++){
				for(int l = 0; l < num_labels; l++){

					SparseVector<int>& f = fs.get_feature_vector(k, l, X, T, i);
					GenVector< std::pair<int, int> > non_zero = f.get_non_zero_elements();
					for(int m = 0; m < non_zero.length(); m++){
						Matrix f_j_i;
						f_j_i.Init(num_labels, num_labels);
						f_j_i.SetZero();
						f_j_i.set(k, l, non_zero[m].second);

						Matrix tmp2;
						DotMultiplyInit(&f_j_i, &M[i],&tmp2);

						Matrix tmp3;
						la::MulInit(alpha[i],tmp2, &tmp3);
						Matrix tmp4;
						la::MulInit(tmp3,beta[i],&tmp4);
						E_F[non_zero[m].first] += tmp4.get(0,0)/Z;
					}
				}
			}
		}

		Vector gradient_contribution;
		la::SubInit(E_F, F, &gradient_contribution);
		la::AddTo(gradient_contribution,gradient);
	}

	std::cerr.precision(6);
	std::cerr << "Iteration " << iter++ << ", Negative LogLikelihood " <<  (-1.0)*LL <<  ", LL1 = " << LL1_persist << ", LL2 = " << LL2_persist << std::endl;
	PrintNonZeroDebug(*gradient);
	la::Scale(-1.0,gradient);
	return (-1.0)*LL;
}

  /**
   * Multiplies A and B entry-wise and Inits a matrix to the result
   * @pre{ A and B are of equal dimensions }
   * (\f$ C \gets A .* B \f$)
   */
  Matrix* DotMultiplyInit(const Matrix* const A, const Matrix* const B,
			  Matrix* C) {
    index_t n_rows = A -> n_rows();
    index_t n_cols = A -> n_cols();

    C -> Init(n_rows, n_cols);

    const double *A_col_j;
    const double *B_col_j;
    double *C_col_j;
    for(index_t j = 0; j < n_cols; j++) {
      A_col_j = A -> GetColumnPtr(j);
      B_col_j = B -> GetColumnPtr(j);
      C_col_j = C -> GetColumnPtr(j);
      for(index_t i = 0; i < n_rows; i++) {
	C_col_j[i] = A_col_j[i] * B_col_j[i];
      }
    }

    return C;
  }


void LogMultiplyInit(const Matrix& log_A, const Matrix& log_B, Matrix* log_C){

	log_C->Init(log_A.n_rows(),log_B.n_cols());
	log_C->SetZero();
	for(int i = 0; i < log_C->n_cols(); i++){
		for(int j = 0; j < log_C->n_rows(); j++){
			Vector logs;
			logs.Init(log_A.n_cols());
			for(int k = 0; k < logs.length(); k++)
				logs[k] = log_A.get(j,k) + log_B.get(k,i);
			log_C->set(j,i,log_sum_exp(logs));
		}
	}
}

void LogMaxInit(const Matrix& log_A, const Matrix& log_B, Matrix* log_C, Matrix* choices){

	log_C->Init(log_A.n_rows(),log_B.n_cols());
	log_C->SetZero();
	choices->Init(log_A.n_rows(),log_B.n_cols());
	choices->SetZero();
	for(int i = 0; i < log_C->n_cols(); i++){
		for(int j = 0; j < log_C->n_rows(); j++){
			double max = -DBL_MAX;
			int best = 0;
			for(int k = 0; k < log_A.n_cols(); k++)
				if(max < log_A.get(j,k)  + log_B.get(k,i)) {
					max = log_A.get(j,k)  + log_B.get(k,i);
					best = k;
				}
			log_C->set(j, i, max);
			choices->set(j, i, best);
		}
	}
}

double log_sum_exp(const Vector& logs){

	double min = DBL_MAX;
	for(int i = 0; i < logs.length(); i++)
		if(logs[i] < min)
			min = logs[i];
	double result = 0;
	result = result + min;
	double tmp = 0;
	for(int i = 0; i < logs.length(); i++){
		tmp += exp(logs[i] - min);
	}
	assert(tmp > 0);
	result += log(tmp);
	return result;
}

void PrintNonZeroDebug(const Vector& v){

	std::cout << "------- VECTOR  NON ZERO  ------ " << std::endl;
	for(int i = 0; i < v.length(); i++)
		if(abs(v[i] - 0.0) > 1E-5)
			std::cout << i << " : " << v[i] << " ";
	std::cout << std::endl;

}

void save_lambda(const Vector& lambda, const char* filename) {

	ofstream f;
	f.open(filename);
	f << lambda.length() << endl;
	for(int i = 0; i < lambda.length(); i++)
		f << lambda[i] << std::endl;	
	f.close();
}


void save_featureset(Featureset& fs, const char* filename) {

	
	char* filenames[NUMBER_FILE_CHUNKS];
	for(int i = 0; i < NUMBER_FILE_CHUNKS; i++) {
		filenames[i] = (char*) malloc(sizeof(char)*8);
		 std::stringstream ss;
		 ss << i;
		 strcpy(filenames[i], ss.str().c_str()); 
	}
	fs.save((const char**)filenames);
}

void load_featureset(Featureset& fs, const char* filename) {

	char* filenames[NUMBER_FILE_CHUNKS];
	for(int i = 0; i < NUMBER_FILE_CHUNKS; i++) {
		filenames[i] = (char*) malloc(sizeof(char)*8);

		 std::stringstream ss;
		 ss << i;
		 strcpy(filenames[i], ss.str().c_str()); 
	}
	fs.load((const char**)filenames);
}
