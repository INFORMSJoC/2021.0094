// stdafx.cpp : 只包括标准包含文件的源文件
// Energy_efficient_linear_model.pch 将作为预编译头
// stdafx.obj 将包含预编译类型信息

#include "stdafx.h"

int t_r(int i, int q)
{	
	int s_output;
	
	if (q==0)
	{
		s_output=1;
	}
	else if (q==1)
	{
		s_output=3;
	}
	else if (q==2)
	{
		s_output=4;
	}
	else if (q==3)
	{
		s_output=5;
	}
	else if (q==4)
	{
		s_output=6;
	}
	return s_output;
}
int e_r(int i, int q)
{
	int e_consum;
	
	if (q==0)
	{
		e_consum=0;
	}
	else if (q==1)
	{
		e_consum=6;
	}
	else if (q==2)
	{
		e_consum=5;
	}
	else if (q==3)
	{
		e_consum=4;
	}
	else if (q==4)
	{
		e_consum=3;
	}
	return e_consum;
}
float max_two(float a, float b)
{
	float c;
	if(a-b>0)
		c=a;
	else
		c=b;
	return c;
}

int find(int num[], int n)
{
	int out;
	for(int i=0; i<train_exist; i++)
	{
		if(num[i]*T_stamp==n)
		{
			out=1;
			break;
		}
		else
		{
			out=0;
		}
	}
	return out;
}
// TODO: 在 STDAFX.H 中
// 引用任何所需的附加头文件，而不是在此文件中引用
