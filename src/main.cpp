/****************************************************************************
 *   Copyright (c) 2018 Yeap Bing Cheng. All rights reserved.
 *
 *   Author: Yeap Bing Cheng <ybingcheng@gmail.com> 
 * 
 ****************************************************************************/
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#include "base.h"

int main(int argc, char *argv[])
{
    CorrectBase base;
    base.serialConnect();
    // base.gpsConnect();
    // base.operateBase();

    return 0;
}