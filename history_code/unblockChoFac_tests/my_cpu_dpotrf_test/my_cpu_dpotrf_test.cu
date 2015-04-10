#include<iostream>
#include<cstdlib>
#include<iomanip>
#include<cmath>
#include<ctime>
#include"cblas.h"
#include"cublas_v2.h"

#include<curand.h>

#include "lapacke.h"
#include "blas.h"
#include "papi.h"

using namespace std;

void matrixGenerator_gpu(char uplo, double * matrix, double * result, int N, int B);
__global__ void matrixDiagonalizeAndScale(double * matrix, int ld, char uplo, double alpha, double beta);

//cpu version
void POTF2_CPU(char uplo, double * matrix, int ld, int B){
    if(uplo == 'u'){
        for(int i = 0; i<B;i++){
            matrix[i*ld+i] = sqrt(matrix[i*ld+i]);
            for(int j=i+1;j<B;j++){
                matrix[j*ld+i] /=matrix[i*ld+i];
            }
            for(int j=i+1;j<B;j++){
                for(int k=i+1;k<j+1;k++){
                    matrix[j*ld+k]-=matrix[j*ld+i]*matrix[k*ld+i];
                }
            }
        }
    }
    if(uplo == 'l'){
        for(int i = 0; i<B;i++){
            matrix[i*ld+i] = sqrt(matrix[i*ld+i]);
            for(int j=i+1;j<B;j++){
                matrix[i*ld+j] /=matrix[i*ld+i];
            }
            for(int j=i+1;j<B;j++){
                for(int k=i+1;k<j+1;k++){
                    matrix[k*ld+j]-=matrix[i*ld+j]*matrix[i*ld+k];
                }
            }
        }
    }
}

void test(int c, int N, int B, char uplo, float * real_time, float * proc_time, long long * flpins, float * mflops){
    double * matrix;
    double * result;
    double * temp = new double[N*N]();
    cudaMalloc((void**)&matrix,N*N*sizeof(double));
    cudaMalloc((void**)&result,N*N*sizeof(double));
    
    matrixGenerator_gpu(uplo, matrix, result, N, B);

    if(PAPI_flops(real_time, proc_time, flpins, mflops)<PAPI_OK){
        cout<<"PAPI ERROR"<<endl;
        return;
    }

    cudaMemcpy2D(temp,
                 N*sizeof(double),
                 matrix,
                 N*sizeof(double),
                 N*sizeof(double),
                 N,
                 cudaMemcpyDeviceToHost);
    
    POTF2_CPU(uplo, temp, N, N);
    
    cudaMemcpy2D(matrix,
                 N*sizeof(double),
                 temp,
                 N*sizeof(double),
                 N*sizeof(double),
                 N,
                 cudaMemcpyHostToDevice);

    if(PAPI_flops( real_time, proc_time, flpins, mflops)<PAPI_OK){
        cout<<"PAPI ERROR"<<endl;
        return;
    }

    delete[] temp;
    cudaFree(matrix);
    cudaFree(result);
    PAPI_shutdown();
}

int main(){
    float real_time = 0.0;
    float proc_time = 0.0;
    long long flpins = 0.0;
    float mflops = 0.0;
    
    float total_real_time = 0.0;
    float total_proc_time = 0.0;
    long long total_flpins = 0.0;
    float total_mflops = 0.0;
    
    int TEST_NUM = 1000;
    
    for(int n=2;n<1025;n*=2){
        for(int i=0;i<TEST_NUM;i++){
            test(0,n,2,'u',&real_time,&proc_time,&flpins,&mflops);
            total_real_time += real_time;
            total_proc_time += proc_time;
            total_flpins += flpins;
            total_mflops += mflops;
        }
        cout<<"CPU";
        cout<<"Size:"<<n<<"---Real_time:"<<total_real_time/(double)TEST_NUM<<"---"<<"Proc_time:"<<total_proc_time/(double)TEST_NUM<<"---"<<"Total flpins:"<<total_flpins/(double)TEST_NUM<<"---"<<"MFLOPS:"<<total_mflops/(double)TEST_NUM<<endl;
    }
    
}


void matrixGenerator_gpu(char uplo, double * matrix, double * result, int N, int B){
    double a = 10.0;
    //initialize cublas
    cublasHandle_t handle;
    cublasCreate(&handle);
    
    //initialize curand
    curandGenerator_t gen;
    curandCreateGenerator(&gen,CURAND_RNG_PSEUDO_DEFAULT);
    curandSetPseudoRandomGeneratorSeed(gen,10ULL);
    //generate random number in range (0,1] on result using curand
    curandGenerateUniformDouble(gen,result,N*N);
    cudaDeviceSynchronize();
    //  printMatrix_gpu(result,N);
    matrixDiagonalizeAndScale<<<dim3(N/B,N/B),dim3(B,B)>>>(result, N, uplo, a,1);
    cudaDeviceSynchronize();
    //printMatrix_gpu(result,N);
    //do matrix-matrix multiplcation using cublas
    cudaMemset(matrix,0,N*N*sizeof(double));
    
    
    double alpha = 1.0;
    double beta = 1.0;
    if(uplo == 'u'){
        cublasDgemm(handle,CUBLAS_OP_T,CUBLAS_OP_N,N,N,N,&alpha,result,N,result,N,&beta,matrix,N);
    }
    else if(uplo == 'l'){
        cublasDgemm(handle,CUBLAS_OP_N,CUBLAS_OP_T,N,N,N,&alpha,result,N,result,N,&beta,matrix,N);
    }
    cudaDeviceSynchronize();
    //    printMatrix_gpu(matrix,N);
    matrixDiagonalizeAndScale<<<dim3(N/B,N/B),dim3(B,B)>>>(matrix, N, uplo, 1.0,0);
    cudaDeviceSynchronize();
    // printMatrix_gpu(matrix,N);
}

__global__ void matrixDiagonalizeAndScale(double * matrix, int ld, char uplo, double alpha, double beta){
    int col = threadIdx.x+blockIdx.x*blockDim.x;
    int row = threadIdx.y+blockIdx.y*blockDim.y;
    if(uplo == 'u'){
        if(row<col+1){
            matrix[col*ld+row] = int(matrix[col*ld+row]*alpha+beta);
        }
        else{
            matrix[col*ld+row] = int(0.0);
        }
    }
    else{
        if(col<row+1){
            matrix[col*ld+row] = int(matrix[col*ld+row]*alpha+beta);
        }
        else{
            matrix[col*ld+row] = int(0.0);
        }
    } 
}
