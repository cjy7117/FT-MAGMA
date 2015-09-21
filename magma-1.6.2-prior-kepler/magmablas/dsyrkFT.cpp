#include"FT.h"
#include<iostream>
using namespace std;
//dsyrk with FT
/*
__global__ void detectAndCorrectForSyrk(double * C, int ldc,
		double * chksumC1, int incC1, double * chksumC2, int incC2,
		double * chkC1, int incC1_2, double * chkC2, int incC2_2){
	//determin the reponsisble column 
	int col = threadIdx.x;
	double diff = abs(*(chkC1+col*incC1_2)-*(chksumC1+col*incC1));
	if(diff>0.1){
		double diff2=abs(*(chkC2+col*incC2_2)-*(chksumC2+col*incC2));
		int row = (int)round(diff2/diff)-1;
		*(C+row+col*ldc) += *(chksumC1+col*incC1)-*(chkC1+col*incC1_2);
	}
}
*/

/**
 * n: number of row of A
 * m: number of col of A
 */
void dsyrkFT(int n, int m, double * A, int lda, double * C, int ldc,
		double * checksumA, int checksumA_ld,
		double * checksumC, int checksumC_ld,
		double * vd, int vd_ld,
		double * v, int v_ld,
		double * chk1, int chk1_ld,
		double * chk2, int chk2_ld,
		magma_queue_t * streams,
		bool FT, bool DEBUG){
	
	/*		   m				n
	 * ******************   *********
	 * *		A		* =>*	C	* n
	 * *				* 	*		*
	 * ******************	*********
	 */
	
	
	
	
	if (FT) {
//		//verify A before use
//		//reclaculate checksums of A on GPU
//		magmablasSetKernelStream(streams[2]);
//		magma_dgemv(MagmaTrans, n, m, MAGMA_D_ONE,
//				A, lda, vd, vd_ld, MAGMA_D_ZERO, chk1, chk1_ld );
//		magmablasSetKernelStream(streams[3]);
//		magma_dgemv(MagmaTrans, n, m, MAGMA_D_ONE,
//				A, lda, vd + 1, vd_ld, MAGMA_D_ZERO, chk2, chk2_ld );
//		
//		//handle error - to be finished
//		
//		if (DEBUG) {
//			cout<<"recalculated checksum of A before dsyrk:"<<endl;
//			printMatrix_gpu(chk1, chk1_ld, 1, m);
//			printMatrix_gpu(chk2, chk2_ld, 1, m);
//		
//			cout<<"updated checksum of A before dsyrk:"<<endl;
//			printMatrix_host(checksumA, checksumA_ld, 2, m);
//		}
//		
		
	}
	
	double negone = -1;
	double one = 1;
	double zero = 0;
	
	//if (FT) {
//		magmablasSetKernelStream(streams[1]);
//		magma_dgemm(
//				MagmaNoTrans, MagmaTrans,
//				n, n, m,
//				MAGMA_D_ONE * (-1),
//				A, lda, A, lda,
//				MAGMA_D_ONE,
//				C, ldc );
////	} else {
//		magma_dsyrk(MagmaLower, MagmaNoTrans, n, m,
//						MAGMA_D_ONE * (-1), A, lda,
//						MAGMA_D_ONE,     C, ldc);
//	}
	
	if(FT){
		//update checksums on GPU
		magmablasSetKernelStream(streams[4]);
		magma_dgemm(
					MagmaNoTrans, MagmaTrans,
					2, n, m,
					MAGMA_D_ONE * (-1),
					checksumA, checksumA_ld, A, lda,
					MAGMA_D_ONE,
					checksumC, checksumC_ld );
	}
}