/**
 * @file Inter.cpp
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-05-22
 * 
 * @copyright Copyright (c) 2025
 *    Interpolate based on a table.
 */ 
#include "Interp.h"

// - - - - - - - - - - - - - - - - - - - -
// Construct a new interpolation, based on given table.
//   Note: The table is NOT copied - it must remain available
// Param _tabX - the X table
// Param _tabY - the Y table
// Param _tabLen - how many entries in tabX and tabY
//
Interp::Interp( double *_tabx, double *_taby, int _tabLen)
{
    tabx=_tabx;  // Points  to an array of X values
    taby=_taby;  // Points to an corresponding list of Y values.    
    tabLen=_tabLen;    // How many values in list?
    yIncr = (taby[0] < taby[1]);
}


// - - - - - - -Destructor - - - - - - - - - - - - -
Interp::~Interp()
{
    return;
}


/**
 * @brief Find the MINIMUM position in the table for X.
 *   If X is less than tabx[0], we return 0 - (Interpolate
 *       using first two entries in the system)
 *       
 *   If X is greater than max tabx, we return tabLen-2 (interpolate 
 *      using last two entries in system)
 *  the return value is the index is the low end of the range where we placed X
 *   
 * @param givenX   - the x-value we are searching for
 * @return int     - the index of the minimum position in the table
 */
int Interp::findXIdx(double givenX)
{
    int res;

    // deal with edge cases
    if (givenX <= tabx[0]) return(0);
    if (givenX >= tabx[tabLen-1]) return (tabLen-2);

    // Search the table
    for (res=tabLen-1; res>0; res--)
    {
        if ( givenX  >= tabx[res]) break;
    }
    return(res);
}


/**
 * @brief Find  the table index where the given Y fits.
 * (This does NOT assumem the table is increasing!)
 * 
 * We return the lower index of the interval that contains that
 * Y value.
 * 
 * If the Y value is outside the Y limits, we return the lower
 * index of the interval nearest that value.
 * 
 * In all cases, we can interpolate between index x and index x+1.
 * 
 * @param givenY  - the Y value we want
 * @return int    - the index of the minimum position in the table
 */
int Interp::findYIdx(double givenY)
{
    int res;

    // Deal with edge cases...
    if (yIncr)
    { // y values are increaasing
        if (givenY <= taby[0])             return (0);
        if (givenY >= taby[tabLen - 1])    return (tabLen - 2);
    } else {
        if (givenY <= taby[tabLen - 1])    return (tabLen-2);
        if (givenY >= taby[0])             return (0);
    }

    // Okay, we have to search for it...
    for (res = tabLen - 1; res > 0; res--)
    {
        if (yIncr)
        {
            if (givenY >= tabx[res])
                break;
        }
        else
        {
            if (givenY <= taby[res])
                break;
        }
    }
    return (res);
}


/**
 * @brief F(x) - given x what is the y value?
 *    Given 'x', what is the approximate value of 'y' ?
 *  This uses the nearest two values of X to determine
 * the value of Y to return.
 */
double Interp::interpolate(double x)
{
    int idx = findXIdx(x);
    double result;
    result = (x - tabx[idx]) * (taby[idx + 1] - taby[idx]) / (tabx[idx + 1] - tabx[idx]) + taby[idx];
    return (result);
}


/**
 * @brief Reverse interpolation - x=F(y) - given y, what was the X value?
 * 
 * @param y         - the target result
 * @return double   - The needed X value
 */
double Interp::revInterpolate(double y)
{
    int idx = findYIdx(y);
    double result;
     result = (y - taby[idx]) * (tabx[idx + 1] - tabx[idx]) / (taby[idx + 1] - taby[idx]) + tabx[idx];
    return (result);
}