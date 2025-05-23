/**
 * @file Interp.h - interpolate a value, based on a table
 * @author Doug Fajardo
 * @brief Given a table, determine the actual value of y, given any x
 * @version 0.1
 * @date 2025-05-22
 * 
 * @copyright Copyright (c) 2025
 * 
 * Perform Linear interpolation on a table
 */
#pragma once


// - - - - - - How to define a table for LINEAR interpolation- - - - 
// Note that (1) there must be at least 2 entries in the table (3 for )
//           (2) tabx values MUST be increasing in value. 
//           (3) for each tabx, there must be a matching taby.
//           (4) taby values MUST be monitonic, although they can be 
//                   either increasing or decreasing in value.
//    
// - - - - - - - The Interpolation class - - - - - - - 
class Interp
{
private:
    double *tabx;  // Points  to an array of X values
    double *taby;  // Points to an corresponding list of Y values.
    int tabLen;    // How many values in list?
    bool yIncr;     // is y monotonicly INCREASING (true) or DECREASING(false).


    int findXIdx(double givenX);
    int findYIdx(double givenY);


public:
    Interp( double *tabX, double *taby, int tabLen);
    ~Interp();
    double interpolate(double x);
    double revInterpolate(double y);
};