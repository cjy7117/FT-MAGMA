/*
    -- MAGMA (version 2.0.2) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date May 2016

       @generated from testing/testing_zgeqlf.cpp normal z -> s, Mon May  2 23:31:16 2016

*/

// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// includes, project
#include "flops.h"
#include "magma_v2.h"
#include "magma_lapack.h"
#include "testings.h"


/* ////////////////////////////////////////////////////////////////////////////
   -- Testing sgeqlf
*/
int main( int argc, char** argv)
{
    TESTING_INIT();

    const float             d_neg_one = MAGMA_D_NEG_ONE;
    const float             d_one     = MAGMA_D_ONE;
    const float c_neg_one = MAGMA_S_NEG_ONE;
    const float c_one     = MAGMA_S_ONE;
    const float c_zero    = MAGMA_S_ZERO;
    const magma_int_t        ione      = 1;
    
    real_Double_t    gflops, gpu_perf, gpu_time, cpu_perf=0, cpu_time=0;
    float           Anorm, error=0, error2=0;
    float *h_A, *h_R, *tau, *h_work, tmp[1];
    magma_int_t M, N, n2, lda, lwork, info, min_mn, nb;
    magma_int_t ISEED[4] = {0,0,0,1};
    magma_int_t status = 0;

    magma_opts opts;
    opts.parse_opts( argc, argv );

    float tol = opts.tolerance * lapackf77_slamch("E");
    
    printf("%%   M     N   CPU Gflop/s (sec)   GPU Gflop/s (sec)   |L - Q^H*A|   |I - Q^H*Q|\n");
    printf("%%==============================================================================\n");
    for( int itest = 0; itest < opts.ntest; ++itest ) {
        for( int iter = 0; iter < opts.niter; ++iter ) {
            M = opts.msize[itest];
            N = opts.nsize[itest];
            min_mn = min(M, N);
            lda    = M;
            n2     = lda*N;
            nb     = magma_get_sgeqlf_nb( M, N );
            gflops = FLOPS_SGEQLF( M, N ) / 1e9;
            
            // query for workspace size
            lwork = -1;
            lapackf77_sgeqlf(&M, &N, NULL, &M, NULL, tmp, &lwork, &info);
            lwork = (magma_int_t)MAGMA_S_REAL( tmp[0] );
            lwork = max( lwork, N*nb );
            lwork = max( lwork, 2*nb*nb);
            
            TESTING_MALLOC_CPU( tau,    float, min_mn );
            TESTING_MALLOC_CPU( h_A,    float, n2     );
            TESTING_MALLOC_CPU( h_work, float, lwork  );
            
            TESTING_MALLOC_PIN( h_R,    float, n2     );
            
            /* Initialize the matrix */
            lapackf77_slarnv( &ione, ISEED, &n2, h_A );
            lapackf77_slacpy( MagmaFullStr, &M, &N, h_A, &lda, h_R, &lda );
            
            /* ====================================================================
               Performs operation using MAGMA
               =================================================================== */
            gpu_time = magma_wtime();
            magma_sgeqlf( M, N, h_R, lda, tau, h_work, lwork, &info);
            gpu_time = magma_wtime() - gpu_time;
            gpu_perf = gflops / gpu_time;
            if (info != 0) {
                printf("magma_sgeqlf returned error %d: %s.\n",
                       (int) info, magma_strerror( info ));
            }
            
            /* =====================================================================
               Check the result, following zqlt01 except using the reduced Q.
               This works for any M,N (square, tall, wide).
               =================================================================== */
            if ( opts.check ) {
                magma_int_t ldq = M;
                magma_int_t ldl = min_mn;
                float *Q, *L;
                float *work;
                TESTING_MALLOC_CPU( Q,    float, ldq*min_mn );  // M by K
                TESTING_MALLOC_CPU( L,    float, ldl*N );       // K by N
                TESTING_MALLOC_CPU( work, float,             min_mn );
                
                // copy M by K matrix V to Q (copying diagonal, which isn't needed) and
                // copy K by N matrix L
                lapackf77_slaset( "Full", &min_mn, &N, &c_zero, &c_zero, L, &ldl );
                if ( M >= N ) {
                    // for M=5, N=3: A = [ V V V ]  <= V full block (M-N by K)
                    //          K=N      [ V V V ]
                    //                   [ ----- ]
                    //                   [ L V V ]  <= V triangle (N by K, copying diagonal too)
                    //                   [ L L V ]  <= L triangle (K by N)
                    //                   [ L L L ]
                    magma_int_t M_N = M - N;
                    lapackf77_slacpy( "Full",  &M_N, &min_mn,  h_R,      &lda,  Q,      &ldq );
                    lapackf77_slacpy( "Upper", &N,   &min_mn, &h_R[M_N], &lda, &Q[M_N], &ldq );
                    
                    lapackf77_slacpy( "Lower", &min_mn, &N,   &h_R[M_N], &lda,  L,      &ldl );
                }
                else {
                    // for M=3, N=5: A = [ L L | L V V ] <= V triangle (K by K)
                    //     K=M           [ L L | L L V ] <= L triangle (K by M)
                    //                   [ L L | L L L ]
                    //                     ^^^============= L full block (K by N-M)
                    magma_int_t N_M = N - M;
                    lapackf77_slacpy( "Upper", &M, &min_mn,  &h_R[N_M*lda], &lda,  Q,          &ldq );
                    
                    lapackf77_slacpy( "Full",  &min_mn, &N_M, h_R,          &lda,  L,          &ldl );
                    lapackf77_slacpy( "Lower", &min_mn, &M,  &h_R[N_M*lda], &lda, &L[N_M*ldl], &ldl );
                }
                
                // generate M by K matrix Q, where K = min(M,N)
                lapackf77_sorgql( &M, &min_mn, &min_mn, Q, &ldq, tau, h_work, &lwork, &info );
                assert( info == 0 );
                
                // error = || L - Q^H*A || / (N * ||A||)
                blasf77_sgemm( "Conj", "NoTrans", &min_mn, &N, &M,
                               &c_neg_one, Q, &ldq, h_A, &lda, &c_one, L, &ldl );
                Anorm = lapackf77_slange( "1", &M,      &N, h_A, &lda, work );
                error = lapackf77_slange( "1", &min_mn, &N, L,   &ldl, work );
                if ( N > 0 && Anorm > 0 )
                    error /= (N*Anorm);
                
                // set L = I (K by K identity), then L = I - Q^H*Q
                // error = || I - Q^H*Q || / N
                lapackf77_slaset( "Upper", &min_mn, &min_mn, &c_zero, &c_one, L, &ldl );
                blasf77_ssyrk( "Upper", "Conj", &min_mn, &M, &d_neg_one, Q, &ldq, &d_one, L, &ldl );
                error2 = safe_lapackf77_slansy( "1", "Upper", &min_mn, L, &ldl, work );
                if ( N > 0 )
                    error2 /= N;
                
                TESTING_FREE_CPU( Q    );  Q    = NULL;
                TESTING_FREE_CPU( L    );  L    = NULL;
                TESTING_FREE_CPU( work );  work = NULL;
            }
            
            /* =====================================================================
               Performs operation using LAPACK
               =================================================================== */
            if ( opts.lapack ) {
                cpu_time = magma_wtime();
                lapackf77_sgeqlf( &M, &N, h_A, &lda, tau, h_work, &lwork, &info );
                cpu_time = magma_wtime() - cpu_time;
                cpu_perf = gflops / cpu_time;
                if (info != 0) {
                    printf("lapack_sgeqlf returned error %d: %s.\n",
                           (int) info, magma_strerror( info ));
                }
            }
            
            /* =====================================================================
               Print performance and error.
               =================================================================== */
            printf("%5d %5d   ", (int) M, (int) N );
            if ( opts.lapack ) {
                printf( "%7.2f (%7.2f)", cpu_perf, cpu_time );
            }
            else {
                printf("  ---   (  ---  )" );
            }
            printf( "   %7.2f (%7.2f)   ", gpu_perf, gpu_time );
            if ( opts.check ) {
                bool okay = (error < tol && error2 < tol);
                status += ! okay;
                printf( "%11.2e   %11.2e   %s\n", error, error2, (okay ? "ok" : "failed") );
            }
            else {
                printf( "    ---\n" );
            }
            
            TESTING_FREE_CPU( tau    );
            TESTING_FREE_CPU( h_A    );
            TESTING_FREE_CPU( h_work );
            
            TESTING_FREE_PIN( h_R    );
            fflush( stdout );
        }
        if ( opts.niter > 1 ) {
            printf( "\n" );
        }
    }

    opts.cleanup();
    TESTING_FINALIZE();
    return status;
}
