/* Sequential mss implementations 
 * Author: Raphaël Dang-Nhu
 * DAte: May 7th */


/* This function computes the maximum prefix sum of an array with superaccumulators */
void sequential_mss_superacc(double*,int,double*,int* posl,int* posr);

/* This function computes the maximum prefix sum of an array with doubles */
void sequential_mss_double(double*,int,double*,int* posl,int* posr);

/* This function computes the maximum prefix sum in a lazy way
 * Last argument is for optional optimization method */
void sequential_mss_lazy(double*,int,double*,int* posl,int* posr);