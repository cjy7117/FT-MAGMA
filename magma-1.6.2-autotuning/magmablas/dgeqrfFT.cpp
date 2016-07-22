#include "FT.h"
#include <iostream>

using namespace std;

//QR Factorization with FT on CPU using ACML
//m: number of row
//n: number of col


void dgeqrfFT( int m, int n, double * A, int lda, double * tau, double * work, int lwork, int * info,
			   ABFTEnv * abftEnv, 
			   bool FT , bool DEBUG, bool VERIFY) {

	if (DEBUG) {
		cout << "[DGEQRF] input matrix before factorization" << endl;
		printMatrix_host(A, lda, m, n, 4, 4);

		cout << "[DGEQRF] column checksum before factorization" << endl;
		printMatrix_host(abftEnv->col_hchk, abftEnv->col_hchk_ld, (m / abftEnv->chk_nb) * 2, n, 2, 4);

		cout << "[DGEQRF] row checksum before factorization" << endl;
		printMatrix_host(abftEnv->row_hchk, abftEnv->row_hchk_ld, m , (n / abftEnv->chk_nb) * 2, 4, 2);

	}

	lapackf77_dgeqrf(&m, &n, A, &lda, tau, work, &lwork, info);

	if (FT) {
		int k = min (m, n);
		char L = 'L';

		for (int i = 0; i < k; i++) {
			double Aii = *(A + i * lda + i);
			*(A + i * lda + i) = 1;

			int pm = m - i;
			int pn = 2;
			int pincv = 1;

			lapackf77_dlarf(&L, &pm, &pn,
                         	A + i * lda + i, &pincv,
                         	tau + i,
                         	abftEnv->row_hchk + i, &(abftEnv->row_hchk_ld),
                         	work );
			*(A + i * lda + i) = Aii;

			double c = -1 * (*(tau + i) * Aii);
			*(abftEnv->row_hchk + i) /= c;
			*(abftEnv->row_hchk + i + 1) /= c;

			//construct v with column checksums
			double * v = new double[m + 2];
			int j = 0;
			while (j < i) {
				v[j] = 0.0;
				j++;
			}
			v[j] = 1; //j = i
			while (j < m) {
				v[j] = *(A + i * lda + j);
				j++;
			}
			v[j] = *(abftEnv->row_hchk + i);
			v[j + 1] = *(abftEnv->row_hchk + i + 1);

			cout << "[DGEQRF] v[" << i << "]:";
			for (int k = 0; k < m + 2; k++)
				cout << v[k] << "\t";
			cout << endl;

		}

	}

	if (DEBUG) {
		cout << "[DGEQRF] input matrix after factorization" << endl;
		printMatrix_host(A, lda, m, n, 4, 4);

		cout << "[DGEQRF] TAU after factorization" << endl;
		for (int i = 0; i < n; i++)
			cout << tau[i] << "\t";
		cout << endl;

		cout << "[DGEQRF] column checksum after factorization" << endl;
		printMatrix_host(abftEnv->col_hchk, abftEnv->col_hchk_ld, (m / abftEnv->chk_nb) * 2, n, 2, 4);

		cout << "[DGEQRF] row checksum after factorization" << endl;
		printMatrix_host(abftEnv->row_hchk, abftEnv->row_hchk_ld, m , (n / abftEnv->chk_nb) * 2, 4, 2);
	}



}