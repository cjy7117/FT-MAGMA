/*
    -- MAGMA (version 2.0.2) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date May 2016

       @generated from sparse-iter/testing/testing_zmatrix.cpp normal z -> s, Mon May  2 23:31:24 2016
       @author Hartwig Anzt
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
#include "magmasparse_internal.h"



/* ////////////////////////////////////////////////////////////////////////////
   -- testing any solver
*/
int main(  int argc, char** argv )
{
    magma_int_t info = 0;
    TESTING_INIT();

    magma_sopts zopts;
    magma_queue_t queue=NULL;
    magma_queue_create( 0, &queue );
    
    real_Double_t res;
    magma_s_matrix Z={Magma_CSR}, A={Magma_CSR}, AT={Magma_CSR}, 
    A2={Magma_CSR}, B={Magma_CSR}, B_d={Magma_CSR};
    
    magma_index_t *comm_i=NULL;
    float *comm_v=NULL;
    magma_int_t start, end;
    
    int i=1;
    CHECK( magma_sparse_opts( argc, argv, &zopts, &i, queue ));

    B.blocksize = zopts.blocksize;
    B.alignment = zopts.alignment;

    while( i < argc ) {
        if ( strcmp("LAPLACE2D", argv[i]) == 0 && i+1 < argc ) {   // Laplace test
            i++;
            magma_int_t laplace_size = atoi( argv[i] );
            CHECK( magma_sm_5stencil(  laplace_size, &Z, queue ));
        } else {                        // file-matrix test
            CHECK( magma_s_csr_mtx( &Z,  argv[i], queue ));
        }

        printf("%% matrix info: %d-by-%d with %d nonzeros\n",
                            int(Z.num_rows), int(Z.num_cols), int(Z.nnz) );
        
        // slice matrix
        CHECK( magma_index_malloc_cpu( &comm_i, Z.num_rows ) );
        CHECK( magma_smalloc_cpu( &comm_v, Z.num_rows ) );
        
        CHECK( magma_smslice( 1, 0, Z, &A2, &AT, &B, comm_i, comm_v, &start, &end, queue ) );    
        magma_sprint_matrix( A2, queue );
        magma_sprint_matrix( AT, queue );
        magma_sprint_matrix( B, queue );
        magma_smfree(&A2, queue );
        magma_smfree(&AT, queue );
        magma_smfree(&B, queue );

        CHECK( magma_smslice( 9, 0, Z, &A2, &AT, &B, comm_i, comm_v, &start, &end, queue ) );    
        magma_sprint_matrix( A2, queue );
        magma_sprint_matrix( AT, queue );
        magma_sprint_matrix( B, queue );
        magma_smfree(&A2, queue );
        magma_smfree(&AT, queue );
        magma_smfree(&B, queue );
        
        CHECK( magma_smslice( 9, 1, Z, &A2, &AT, &B, comm_i, comm_v, &start, &end, queue ) );    
        magma_sprint_matrix( A2, queue );
        magma_sprint_matrix( AT, queue );
        magma_sprint_matrix( B, queue );
        magma_smfree(&A2, queue );
        magma_smfree(&AT, queue );
        magma_smfree(&B, queue );

        CHECK( magma_smslice( 9, 8, Z, &A2, &AT, &B, comm_i, comm_v, &start, &end, queue ) );    
        magma_sprint_matrix( A2, queue );
        magma_sprint_matrix( AT, queue );
        magma_sprint_matrix( B, queue );
        magma_smfree(&A2, queue );
        magma_smfree(&AT, queue );
        magma_smfree(&B, queue );
        
        
        // scale matrix
        CHECK( magma_smscale( &Z, zopts.scaling, queue ));

        // remove nonzeros in matrix
        CHECK( magma_smcsrcompressor( &Z, queue ));
        
        // convert to be non-symmetric
        CHECK( magma_smconvert( Z, &A, Magma_CSR, Magma_CSRL, queue ));
        
        // transpose
        CHECK( magma_smtranspose( A, &AT, queue ));

        // convert, copy back and forth to check everything works

        CHECK( magma_smconvert( AT, &B, Magma_CSR, zopts.output_format, queue ));
        magma_smfree(&AT, queue );
        CHECK( magma_smtransfer( B, &B_d, Magma_CPU, Magma_DEV, queue ));
        magma_smfree(&B, queue );
        CHECK( magma_smcsrcompressor_gpu( &B_d, queue ));
        CHECK( magma_smtransfer( B_d, &B, Magma_DEV, Magma_CPU, queue ));
        magma_smfree(&B_d, queue );
        CHECK( magma_smconvert( B, &AT, zopts.output_format,Magma_CSR, queue ));
        magma_smfree(&B, queue );

        // transpose back
        CHECK( magma_smtranspose( AT, &A2, queue ));
        magma_smfree(&AT, queue );
        CHECK( magma_smdiff( A, A2, &res, queue));
        printf("%% ||A-B||_F = %8.2e\n", res);
        if ( res < .000001 )
            printf("%% tester:  ok\n");
        else
            printf("%% tester:  failed\n");
        
        magma_free_cpu( comm_i );
        magma_free_cpu( comm_v );
        comm_i=NULL;
        comm_v=NULL;
        magma_smfree(&A, queue );
        magma_smfree(&A2, queue );
        magma_smfree(&Z, queue );

        i++;
    }

cleanup:
    magma_free_cpu( comm_i );
    magma_free_cpu( comm_v );
    magma_smfree(&AT, queue );
    magma_smfree(&A, queue );
    magma_smfree(&B, queue );
    magma_smfree(&B_d, queue );
    magma_smfree(&A2, queue );
    magma_smfree(&Z, queue );
    magma_queue_destroy( queue );
    TESTING_FINALIZE();
    return info;
}
