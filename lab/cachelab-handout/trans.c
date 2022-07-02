/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void trans_block_size8(int begin_x, int begin_y, int M, int N, int A[N][M], int B[M][N]);
void trans_block_size4(int begin_x, int begin_y, int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    
    // Set each block has the length of 8 int
    int block_len;
    // For 32x32 matrix, the block size is set to be 8
    if (N == 32 && M == 32) {
    block_len = 8;
    // Other two cases(64x64 and 61x67), the block size is set to be 4
    } else {
    block_len = 4;
    }
    // Row scan
    for (i = 0; i < N; i += block_len) {
        // Column scan
        for (j = 0; j < M; j += block_len) {
            if (N == 32 && M == 32) {
            trans_block_size8(i, j, M, N, A, B);
            } else {
            trans_block_size4(i, j, M, N, A, B);
            }
        }
    }
}

// Transpose a small block of matrix. The block is 8x8.
void trans_block_size8(int begin_x, int begin_y, int M, int N, int A[N][M], int B[M][N]) {
    for (int i = begin_x; i < begin_x + 8; i++) {
        // Retrive all the elements in a cache line at once!
        // @Note: We need to first retrive all the precious data from the cache line. By doing so, we
        // then don't have to worry about whether the cache line is used, becasue we stored them in
        // registers. If we don't do this, but use a for loop and copy the elemnts in this form: B[i][j] = A[j][i]
        // , we access the A array and B array altenately. Then one cache line may be occupied by A and B altenately,
        // causing unnecessary cache misses.
        int cur_y_idx = begin_y;
        int r0 = A[i][cur_y_idx++];
        int r1 = A[i][cur_y_idx++];
        int r2 = A[i][cur_y_idx++];
        int r3 = A[i][cur_y_idx++];
        int r4 = A[i][cur_y_idx++];
        int r5 = A[i][cur_y_idx++];
        int r6 = A[i][cur_y_idx++];
        int r7 = A[i][cur_y_idx++];

        // Then, assign all the elements to B at once too.
        int cur_x_idx = begin_y;
        B[cur_x_idx++][i] = r0;
        B[cur_x_idx++][i] = r1;
        B[cur_x_idx++][i] = r2;
        B[cur_x_idx++][i] = r3;
        B[cur_x_idx++][i] = r4;
        B[cur_x_idx++][i] = r5;
        B[cur_x_idx++][i] = r6;
        B[cur_x_idx++][i] = r7;
    }
}

// Transpose a small block of matrix. The block is 4x4.
void trans_block_size4(int begin_x, int begin_y, int M, int N, int A[N][M], int B[M][N]) {
    for (int i = begin_x; i < begin_x + 4 && i < N; i++) {
        // Retrive all the elements in a cache line at once!
        int cur_y_idx = begin_y;
        int cur_x_idx = begin_y;
        if (begin_y + 3 <= M) {
            int r0 = A[i][cur_y_idx++];
            int r1 = A[i][cur_y_idx++];
            int r2 = A[i][cur_y_idx++];
            int r3 = A[i][cur_y_idx++];

            // Then, assign all the elements to B at once too.
            B[cur_x_idx++][i] = r0;
            B[cur_x_idx++][i] = r1;
            B[cur_x_idx++][i] = r2;
            B[cur_x_idx++][i] = r3;
        } else if (begin_y + 1 == M) {
            int r0 = A[i][cur_y_idx++];

            // Then, assign all the elements to B at once too.
            B[cur_x_idx++][i] = r0;
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
