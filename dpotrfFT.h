using namespace std;
//Cholesky Factorization with FT on CPU using ACML
double get(double * matrix, int ld, int n, int i, int j) {
	if (i > ld || j > n)
		cout << "matrix_get_error" << endl;
	return *(matrix + j * ld + i);
}
/**
 * Cholesky factorization with FT on CPU using ACML
 * A: input matrix
 * lda: leading dimension of A
 * n: size of A
 * chksum1: checksum 1
 * inc1: stride between elememts in chksum1
 * chksum2: checksum 2
 * inc2: stride between elememts in chksum2
 */
void dpotrfFT(double * A, int lda, int n, 
				double * chksum, int chksum_ld, double * v, int v_ld, 
				bool FT ) {
	double one = 1;
	double zero = 0;
	double negone = -1;
	
	//cout<<"matrix A before dpotrf:"<<endl;
	//printMatrix_host(A,n,n);
	
	//do Choleksy factorization
	int info;
	dpotrf('L', n, A, n, &info);
	
	if(FT){
	
		//cout<<"matrix A after dpotrf:"<<endl;
		//printMatrix_host(A,n,n);
		
		/*cout<<"checksum on CPU before factorization:"<<endl;
		printVector_host(chksum1, n);
		printVector_host(chksum2, n);
		*/
		//recalculate checksum1 and checksum2
		
		double * chk = new double[n*2];
		int chk_ld = 2;
		//double * chk2 = new double[n];
		dgemm('T','N',2, n, n, one, v, v_ld, A, lda, zero, chk, chk_ld);
		//dgemv('T', n, n, alpha, A, lda, v1, 1, beta, chk1, 1);
		//dgemv('T', n, n, alpha, A, lda, v2, 1, beta, chk2, 1);
		
		//cout<<"recalcuated checksum on CPU after factorization:"<<endl;
		//printMatrix_host(chk, 2, n);
		//printVector_host(chk2, n);
		
		
		//update checksum1 and checksum2
		for (int i = 0; i < n; i++) {
			*(chksum + i*chksum_ld) = *(chksum + i*chksum_ld) / get(A, n, n, i, i);
			*(chksum + i*chksum_ld + 1) = *(chksum + i*chksum_ld + 1) / get(A, n, n, i, i);
			dgemm('N', 'T', 2, n-i-1, 1, negone, chksum + i*chksum_ld, chksum_ld, A + i*lda + i+1, lda, one, chksum + (i+1)*chksum_ld, chksum_ld);
			//daxpy(n-i-1, negone*chksum1[i], A + i*lda + i+1, 1, chksum1 + i+1, 1 );

		}
		/*
		for (int i = 0; i < n; i++) {
			chksum2[i] = chksum2[i] / get(A, n, n, i, i);
			daxpy(n-i-1, negone*chksum2[i], A + i*lda + i+1, 1, chksum2 + i+1, 1 );
		}
		*/
		
		//cout<<"updated checksum on CPU after factorization:"<<endl;
		//printMatrix_host(chksum, 2, n);
		
		//printVector_host(chksum2, n);
	
		//checking error to be finished
		/*for(int i=0;i<n;i++){
			double diff = abs(chk1[i]-chksum1[i]);
			if(diff>0.1){//error detected
				//determine position
				cout<<"Error detected in dpotrf"<<endl;
				double diff2 = abs(chk2[i]-chksum2[i]);
				int j=(int)round(diff2/diff)-1;
				//correct error
				*(A+i*lda+j) += chksum1[i] - chk1[i];
			}
		}
		*/
	}
}