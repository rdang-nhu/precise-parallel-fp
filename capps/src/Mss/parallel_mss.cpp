
#include <iostream>

#include "tbb/parallel_reduce.h"
#include "tbb/blocked_range.h"

#include "parallel_mss.hpp"

#define PRINT 1

using namespace tbb;
using namespace std;


void printA(double* array, int size){
    // printing the array
    cout << "{";
    if(size <= 10){
        for(int i = 0; i < size; i++){
            cout << array[i] << ",";
        }
        cout << "}" << endl;
    }
    else{
        for(int i = 0; i < 10; i++){
            cout << array[i] << ",";
        }
        cout << "... (size: " << size << ")" << endl;
    }

}

void parallel_mss_double(double* array, long size){
    __mss_naive result(array);
    parallel_reduce(blocked_range<long>(0,size),result);
    if(PRINT){
        cout << endl << "Parallel double" << endl;
        printA(array,size);
        result.print_mss();
    }
}

void parallel_mss_interval(double* array, long size){
    __mss_interval result(array);
    parallel_reduce(blocked_range<long>(0,size),result);
    if(PRINT){
        cout << endl << "Parallel interval" << endl;
        printA(array,size);
        result.print_mss();
    }
}

__mss_naive::__mss_naive(double* a) :
    array(a),
    sum(0.),
    mps(0.),
    mts(0.),
    mss(0.),
    posmps(0),
    posmts(0),
    posmssl(0),
    posmssr(0)
{}

__mss_naive::__mss_naive(__mss_naive& x, split) :
    array(x.array),
    sum(0.),
    mps(0.),
    mts(0.),
    mss(0.),
    posmps(0),
    posmts(0),
    posmssl(0),
    posmssr(0)
{}

void __mss_naive::operator()(const blocked_range<long>& r){
    if(posmps == 0){
        posmps = r.begin();
    }
    if(posmts == 0){
        posmts = r.begin();
    }
    if(posmssl == 0 && posmssr == 0){
        posmssl = r.begin();
        posmssr = r.begin();
    }

    for(int i = r.begin(); i != r.end(); i++){

        // Update sum
        sum += array[i];
        
        // Update mts
        mts += array[i];
        if(mts < 0){
            mts = 0.;
            posmts = i+1;
        }
        
        // Update mps
        if(sum >= mps){
            mps = sum;
            posmps = i+1;
        }

        // Update mss
        if(mts >= mss){
            mss = mts;
            posmssl = posmts;
            posmssr = i+1;
        }
    }
}

void __mss_naive::join(__mss_naive& right){

    // Join mss
    double aux = mts + right.mps;
    bool test1 = aux >= mss;
    bool test2 = right.mss >= aux;
    bool test3 = right.mss >= mss;

    if(test2 && test3){
        mss = right.mss;
        posmssl = right.posmssl;
        posmssr = right.posmssr;
    }
    else if (test1 && !test2){
        mss = aux;
        posmssl = posmts;
        posmssr = right.posmps;
    }
    
    // Join mps
    right.mps += sum;
    if(right.mps >= mps){
        mps = right.mps;
        posmps = right.posmps;
    }

    // Join mts
    mts += right.sum;
    if(right.mts >= mts){
        mts = right.mts;
        posmts = right.posmts;
    }
}

void __mss_naive::print_mss(){
    cout << "Sum: " << sum << endl;
    cout << "Mss: " << mss << endl;
    cout << "Left pos: " << posmssl << endl;
    cout << "Right pos: " << posmssr << endl;
    cout << "Mts: " << mts << endl;
    cout << "Pos: " << posmts << endl;
    cout << "Mps: " << mps << endl;
    cout << "Pos: " << posmps << endl;

}

__mss_interval::__mss_interval(double* array_) :
    array(array_),
    posmssl1(0),
    posmssl2(0),
    posmssr1(0),
    posmssr2(0),
    posmts1(0),
    posmts2(0),
    posmps1(0),
    posmps2(0)
{
    sum = in2_create(0,0);
    mss = in2_create(0,0);
    mps = in2_create(0,0);
    mts = in2_create(0,0);
}

__mss_interval::__mss_interval(__mss_interval& x ,split s) :
    array(x.array),
    posmssl1(0),
    posmssl2(0),
    posmssr1(0),
    posmssr2(0),
    posmts1(0),
    posmts2(0),
    posmps1(0),
    posmps2(0)
{
    sum = in2_create(0,0);
    mss = in2_create(0,0);
    mps = in2_create(0,0);
    mts = in2_create(0,0);
}

void __mss_interval::operator()(const blocked_range<long>& r){
    if(posmts1 == 0 && posmts2 == 0){
        posmts1 = r.begin();
        posmts2 = r.begin();
    }
    if(posmps1 == 0 && posmps2 == 0){
        posmps1 = r.begin();
        posmps2 = r.begin();
    }
    if(posmssl1 == 0 && posmssl2 == 0 && posmssr1 == 0 && posmssr2 == 0){
        posmssl1 = r.begin();
        posmssl2 = r.begin();
        posmssr1 = r.begin();
        posmssr2 = r.begin();
    }
    for(int i = r.begin(); i != r.end(); i++){

        // Update sum
        sum = in2_add_double(sum,array[i]);
        
        // Update mts
        mts = in2_add_double(mts,array[i]);
        boolean b = inferior_double(0,mts); 
        if(b == False){
            mts = in2_create(0.,0.);
            posmts1 = i+1;
            posmts2 = i+1;
        } else if (b == Undefined){
            posmts2 = i+1;
        }
        
        // Update mps
        boolean b0 = inferior(mps,sum);
        if(b0 == True){
            mps = sum;
            posmps1 = i+1;
        } else if (b0 == Undefined){
            mps = in2_merge(mps,sum);
            posmps2 = i+1;
        }

        // Update mss
        boolean b1 = inferior(mss,mts);
        if(b1 == True){
            mss = mts;
            posmssl1 = posmts1;
            posmssl2 = posmts2;
            posmssr1 = i+1;
            posmssr2 = i+1;
        } else if(b1 == Undefined){
            mss = in2_merge(mss,mts);
            if(posmssl1 > posmts1) posmssl1 = posmts1; 
            if(posmssl2 < posmts2) posmssl2 = posmts2;
            posmssr2 = i+1;
        }
    }
}

void __mss_interval::join(__mss_interval& right){

}

void __mss_interval::print_mss(){
    cout << "Sum: ";
    print(sum);
    cout << endl << "Mss: ";
    print(mss);
    cout << endl << "Left pos: " << posmssl1 <<", " << posmssl2;
    cout << endl << "Right pos: " << posmssr1 << ", " << posmssr2;
    cout << endl << "Mps: ";
    print(mps);
    cout << endl << "Pos: " << posmps1 << ", " << posmps2;
    cout << endl << "Mts: ";
    print(mts);
    cout << endl << "Pos: " << posmts1 << ", " << posmts2;
    cout << endl;
}

