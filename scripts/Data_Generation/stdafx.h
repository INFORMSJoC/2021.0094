// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once
#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

// Define the parameters here
const int train_exist=10;
const int penlty=1;
const int train_future=5;
const int train_add=3;
const int station=26;
const int T_stamp=720; 
const int h_min=10;
const int t_interval=10;
const int train_capacity=1440;
// const int train_capacity=1114400000;
//const int train_capacity=40;
const int big_M=1000000000;
// Instance 1: 100000
// Instance 2: 1
// Instance 3: 1000
// const int investment_cost=2200000;
// const int investment_cost=1000000;
const int investment_cost= 500000;
//const int investment_cost = 100000000;
const int investment_cost_depot=500000;
const int w_l=10;
//const int state=5;
//const int e_segment[station][state]={1};
const int dwell_min=30;
const int dwell_max=50;
const int t_running=120;
const int t_tunning=120;
const int num_scenario=35; 
const int max_iteration=1; 


int t_r(int i, int q);
int e_r(int i, int q);
float max_two(float a, float b);
int find(int num[], int n);



// TODO: 在此处引用程序需要的其他头文件
