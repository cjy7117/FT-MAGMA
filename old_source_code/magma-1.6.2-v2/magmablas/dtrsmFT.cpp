#include"FT.h"
#include<iostream>
using namespace std;
//TRSM with FT on GPU using cuBLAS
/*
__global__ void detectAndCorrectForTrsm(double * B, int ldb, int n,
		double * chksumB1, int incB1, double * chksumB2, int incB2,
		double * chkB1, int incB1_2, double * chkB2, int incB2_2) {
	//determin the reponsisble column 
	int block = blockIdx.x;
	int col = threadIdx.x;
	double diff = abs(
			*(chkB1 + block + col * incB1_2)
					- *(chksumB1 + block + col * incB1));
	if (diff > 0.1) {
		double diff2 = abs(
				*(chkB2 + block + col * incB2_2)
						- *(chksumB2 + block + col * incB2));
		int row = (int) round(diff2 / diff) - 1;
		*(B + n * block + row + col * ldb) += *(chksumB1 + block + col * incB1)
				- *(chkB1 + block + col * incB1_2);
	}
}
*/
/*
 * m: number of row of B
 * n: number of col of B
 */

void dtrsmFT(int m, int n, double * A, int lda,
		double * B, int ldb, double * checksumB, int checksumB_ld,
		double * vd, int vd_ld,
		double * chk, int chk_ld, bool FT, bool DEBUG) {

//	cout<<"matrix A before dtrsm:"<<endl;
//	 printMatrix_gpu(A,lda*sizeof(double),n,n);
//	 
//	 cout<<"checksum1 of B before dtrsm:"<<endl;
//	 printMatrix_gpu(checksumB1,incB1*sizeof(double),m/n,n);
//	 cout<<"checksum2 of B before dtrsm:"<<endl;
//	 printMatrix_gpu(checksumB2,incB2*sizeof(double),m/n,n);
//	 

	double alpha = 1;
	magma_dtrsm(MagmaRight, MagmaLower, MagmaTrans, MagmaNonUnit,
	                                m, n,
	                                MAGMA_D_ONE, A, lda,
	                                       B, ldb);

//	cublasDtrsm(handle, CUBLAS_SIDE_RIGHT, CUBLAS_FILL_MODE_LOWER, CUBLAS_OP_T,
//			CUBLAS_DIAG_NON_UNIT, m, n, &alpha, A, lda, B, ldb);

	/*cout<<"matrix A after dtrsm:"<<endl;
	 printMatrix_gpu(A,lda*sizeof(double),n,n);
	 */

	if (FT) {
		//recalculate checksum1 and checksum2
		double beta = 0;
		for (int i = 0; i < m; i += n) {
			magma_dgemm( MagmaTrans, MagmaNoTrans,
						 2, n, n,
						 MAGMA_D_ONE, vd, vd_ld,
									B + i, ldb,
						 MAGMA_D_ZERO, chk + (i/n)*2, chk_ld);
		}
		
		//update checksum1 and checksum2
		magma_dtrsm(MagmaRight, MagmaLower, MagmaTrans, MagmaNonUnit,
			                                (m / n) * 2, n,
			                                MAGMA_D_ONE, A, lda,
			                                checksumB, checksumB_ld);

		//cudaStream_t stream1;
		//cublasGetStream(handle, &stream1);
		//cudaStreamSynchronize(stream1);

		if (DEBUG) {
			cout<<"recalculated checksum of B after dtrsm:"<<endl;
			printMatrix_gpu(chk,chk_ld,(m/n)*2,n);

			cout<<"updated checksum of B after dtrsm:"<<endl;
			printMatrix_gpu(checksumB, checksumB_ld, (m / n) * 2, n);
		}
		/*detectAndCorrectForTrsm<<<dim3(m/n),dim3(n)>>>(B, ldb, n,
			checksumB1, incB1, checksumB2, incB2,
			chk1, chk1_ld, chk2, chk2_ld);
		*/
	}
}