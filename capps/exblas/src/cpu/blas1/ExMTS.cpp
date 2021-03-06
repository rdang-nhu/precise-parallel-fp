/*
 *  Copyright (c) 2016 Inria and University Pierre and Marie Curie
 *  All rights reserved.
 */

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <iostream>

#include "ExMTS.hpp"
#include "blas1.hpp"
#include "ExMTS_FPE.hpp"

#ifdef EXBLAS_TIMING
#define iterations 50
#endif


/*
 * Parallel summation using our algorithm
 * If fpe < 2, use superaccumulators only,
 * Otherwise, use floating-point expansions of size FPE with superaccumulators when needed
 * early_exit corresponds to the early-exit technique
 */
__mts exmts(int Ng, double *ag, int fpe, bool early_exit) {
#ifdef EXBLAS_MPI
    int np = 1, p, err;
    MPI_Comm_rank(MPI_COMM_WORLD, &p);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
#endif
    if (fpe < 0) {
        fprintf(stderr, "Size of floating-point expansion should be a positive number. Preferably, it should be in the interval [2, 8]\n");
        exit(1);
    }

    int N;
    double *a;
#ifdef EXBLAS_MPI
    Superaccumulator acc, acc_fin;
    N = Ng / np + Ng % np;

    a = (double *)_mm_malloc(N * sizeof(double), 32);
    if (!a)
        fprintf(stderr, "Cannot allocate memory for per process array\n");

    int i;
    if (p == 0) {
        //distribute
        a = ag;
        ag = ag + N;
        for (i = 1; i < np; i++) {
            err = MPI_Send(ag + (i - 1)  * (N - Ng % np), N - Ng % np, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
            if (err != MPI_SUCCESS)
                fprintf(stderr, "MPI_Send does not word properly %d\n", err);
        }
    } else {
        MPI_Status status;
        err = MPI_Recv(a, N - Ng % np, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
        if (err != MPI_SUCCESS)
            fprintf(stderr, "MPI_Recv does not word properly %d\n", err);
    }
#else
    N = Ng;
    a = ag;
#endif

    // with superaccumulators only
    if (fpe < 2) {
        int nthread = tbb::task_scheduler_init::automatic;
        tbb::task_scheduler_init tbbinit(nthread);
        return ExMTSSuperacc(N, a);
    }
    if (early_exit) {
        if (fpe <= 4)
            return (ExMTSFPE<FPExpansionVectM1<4, FPExpansionTraits<true> > >)(N, a);
        if (fpe <= 6)
            return (ExMTSFPE<FPExpansionVectM1<6, FPExpansionTraits<true> > >)(N, a);
        if (fpe <= 8)
            return (ExMTSFPE<FPExpansionVectM1<8, FPExpansionTraits<true> > >)(N, a);
    } else { // ! early_exit
        if (fpe == 2)
            return (ExMTSFPE<FPExpansionVectM1<2> >)(N, a);
        if (fpe == 3)
            return (ExMTSFPE<FPExpansionVectM1<3> >)(N, a);
        if (fpe == 4)
            return (ExMTSFPE<FPExpansionVectM1<4> >)(N, a);
        if (fpe == 5)
            return (ExMTSFPE<FPExpansionVectM1<5> >)(N, a);
        if (fpe == 6)
            return (ExMTSFPE<FPExpansionVectM1<6> >)(N, a);
        if (fpe == 7)
            return (ExMTSFPE<FPExpansionVectM1<7> >)(N, a);
        if (fpe == 8)
            return (ExMTSFPE<FPExpansionVectM1<8> >)(N, a);
    }

    return {0.0, 0.0};
}

/*
 * Our alg with superaccumulators only
 */
__mts ExMTSSuperacc(int N, double *a) {
    double dacc, dmts;
#ifdef EXBLAS_TIMING
    double t, mint = 10000;
    uint64_t tstart, tend;
    for(int iter = 0; iter != iterations; ++iter) {
    	tstart = rdtsc();
#endif

    TBBlongmts tbbsum(a);
    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, N), tbbsum);
#ifdef EXBLAS_MPI

    tbbsum.acc.Normalize();
        std::vector<int64_t> result(tbbsum.acc.get_f_words() + tbbsum.acc.get_e_words(), 0);
        //MPI_Reduce((int64_t *) &tbbsum.acc.accumulator[0], (int64_t *) &acc_fin.accumulator[0], get_f_words() + get_e_words(), MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&(tbbsum.acc.get_accumulator()[0]), &(result[0]), tbbsum.acc.get_f_words() + tbbsum.acc.get_e_words(), MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        Superaccumulator acc_fin(result);
        dacc = acc_fin.Round();
#else
    dacc = tbbsum.acc.Round();
    dmts = tbbsum.mtsacc.Round();
#endif

#ifdef EXBLAS_TIMING
    tend = rdtsc();
        t = double(tend - tstart) / N;
        mint = std::min(mint, t);
    }
    fprintf(stderr, "%f ", mint);
#endif

    return {dacc,dmts};
}

/**
 * \brief Parallel reduction step
 *
 * \param step step among threads
 * \param tid1 id of the first thread
 * \param tid2 id of the second thread
 * \param acc1 superaccumulator of the first thread
 * \param acc2 superaccumulator of the second thread
 */
inline static void ReductionStep(int step, int tid1, int tid2,
                                 Superaccumulator * acc1, Superaccumulator * acc2,
                                 Superaccumulator * mtsacc1, Superaccumulator * mtsacc2,
                                 int volatile * ready1, int volatile * ready2)
{
    _mm_prefetch((char const*)ready2, _MM_HINT_T0);
    // Wait for thread 2
    while(*ready2 < step) {
        // wait
        _mm_pause();
    }
    double mtsl, mtsr;
    mtsr = mtsacc2->Round();
    mtsacc1->Accumulate(*acc2);
    acc1->Accumulate(*acc2);
    mtsl = mtsacc1->Round();
    if(mtsr > mtsl){
        *mtsacc1 = *mtsacc2;
    }
}

/**
 * \brief Final step of summation -- Parallel reduction among threads
 *
 * \param tid thread ID
 * \param tnum number of threads
 * \param acc superaccumulator
 */
inline static void Reduction(unsigned int tid, unsigned int tnum, std::vector<int32_t>& ready,
                             std::vector<Superaccumulator>& acc,
                             std::vector<Superaccumulator>& mtsacc,
                             int const linesize)
{
    // Custom reduction
    for(unsigned int s = 1; (1 << (s-1)) < tnum; ++s)
    {
        int32_t volatile * c = &ready[tid * linesize];
        ++*c;
        if(tid % (1 << s) == 0) {
            unsigned int tid2 = tid | (1 << (s-1));
            if(tid2 < tnum) {
                //acc[tid2].Prefetch(); // No effect...
                ReductionStep(s, tid, tid2,
                              &acc[tid], &acc[tid2],
                              &mtsacc[tid], &mtsacc[tid2],
                              &ready[tid * linesize],
                              &ready[tid2 * linesize]);
            }
        }
    }
}

template<typename CACHE> __mts ExMTSFPE(int N, double *a) {
    // OpenMP sum+reduction
    int const linesize = 16;    // * sizeof(int32_t)
    int maxthreads = omp_get_max_threads(); // Force maxthreads
    double dacc, dmtsacc;
#ifdef EXBLAS_TIMING
    double t, mint = 10000;
    uint64_t tstart, tend;
    for(int iter = 0; iter != iterations; ++iter) {
        tstart = rdtsc();
#endif
    std::vector<Superaccumulator> acc(maxthreads);
    std::vector<Superaccumulator> mtsacc(maxthreads);
    std::vector<int32_t> ready(maxthreads * linesize);

#pragma omp parallel
    {
        unsigned int tid = omp_get_thread_num();
        unsigned int tnum = omp_get_num_threads();

        CACHE cache(acc[tid], mtsacc[tid]);
        *(int32_t volatile *)(&ready[tid * linesize]) = 0;  // Race here, who cares?

        int l = ((tid * int64_t(N)) / tnum) & ~7ul;
        int r = ((((tid+1) * int64_t(N)) / tnum) & ~7ul) - 1;

        for(int i = l; i < r; i++) {
            asm ("# myloop");
            cache.XAccumulate(a[i]);
        }
        cache.Flush();
        acc[tid].Normalize();
        mtsacc[tid].Normalize();

        Reduction(tid, tnum, ready, acc, mtsacc, linesize);
    }
#ifdef EXBLAS_MPI
    acc[0].Normalize();
        std::vector<int64_t> result(acc[0].get_f_words() + acc[0].get_e_words(), 0);
        MPI_Reduce(&(acc[0].get_accumulator()[0]), &(result[0]), acc[0].get_f_words() + acc[0].get_e_words(), MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        //MPI_Reduce((int64_t *) &acc[0].accumulator[0], (int64_t *) &acc_fin.accumulator[0], get_f_words() + get_e_words(), MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        Superaccumulator acc_fin(result);
        dacc = acc_fin.Round();
#else
    dacc = acc[0].Round();
    dmtsacc = mtsacc[0].Round();
#endif

#ifdef EXBLAS_TIMING
    tend = rdtsc();
        t = double(tend - tstart) / N;
        mint = std::min(mint, t);
    }
    fprintf(stderr, "%f ", mint);
#endif

    return {dacc, dmtsacc};
}

