#include "magma.h"
//#include"FT.h"
using namespace std;
//initialize checksum
void initializeChecksum(double * matrix, int ld, int N, int B, double * vd, int vd_ld, double * chksum, int chksum_ld) {

	//cout<<"checksum vector on GPU:"<<endl;
	//printVector_gpu(vd,B);
	
//	double alpha = 1;
//	double beta = 0;
	for (int i = 0; i < N; i += B) {
//		magma_dgemv(MagmaTrans, B, N, MAGMA_D_ONE, matrix + i, ld, vd, 1, \
//				MAGMA_D_ZERO, chksum + (i / B), chksum_ld);
		
		magma_dgemm(MagmaConjTrans, MagmaNoTrans,
					2, N, B,
					MAGMA_D_ONE, vd, vd_ld,
					matrix + i, ld,
					MAGMA_D_ZERO,chksum + (i / B) * 2, chksum_ld);
		//cublasDgemv(handle, CUBLAS_OP_T, B, N, &alpha, matrix + i, ld, vd, 1, \
				&beta, chksum + (i / B), chksum_ld);
		//cout<<"i="<<i<<endl;
//		printMatrix_gpu(matrix+i,ld*sizeof(double),B,N);
//		printVector_gpu(vd,B);
//		printMatrix_gpu(chksum + (i / B), chksum_ld * sizeof(double), 1, N);
	}
	//printMatrix_gpu(chksum, chksum_ld, 6, N);
			
}