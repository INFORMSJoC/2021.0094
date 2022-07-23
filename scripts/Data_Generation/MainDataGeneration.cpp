// Using "back-up" to reschedule trains 
// CPLEX code and Decomposition approaches 
// By Jiateng Yin at Beijing Jiaotong University

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <ilcplex/ilocplex.h>
#include <cstdlib>
#include <process.h>
#include <math.h>
#include <cmath>
#include <time.h>
#include <ctime>
#include <amp.h>
#include <ppl.h>
#include <algorithm>

typedef IloArray<IloNumVarArray> NumVarMatrix;
typedef IloArray<NumVarMatrix>   NumVar3Matrix;
typedef IloArray<NumVar3Matrix>   NumVar4Matrix;

using namespace concurrency;
ILOSTLBEGIN

// Input
FILE *input_passenger_origin;
FILE *input_passenger_destin;
FILE *input_passenger_existing;
// Output
FILE *output_solution;
FILE *output_solution_traink;
FILE *lower_bound_save_data;
FILE *lagrangian_multipliers;
FILE *space_time_network_flows;
FILE *passenger_flows;
FILE *space_time_network_store;
FILE *alighting_passengers;
FILE *bound_save;
const int Space_Time_Node = T_stamp*station;
typedef struct model_parameters
{
	int qq;
}model_parameters;

int train_esist_position[train_exist] = { 0, 2, 5, 6, 8, 10, 14, 16, 18, 22 };
int total_passenger_demand[num_scenario] = { 0 };
int arc_network[Space_Time_Node][Space_Time_Node] = { 0 };
int passenger_od[station][station][num_scenario];
int index_travel_time_graph[20000][1] = { 0 };
int origin_total[station][num_scenario] = { 0 };
int destination_total[station][num_scenario] = { 0 };
int Space_Time_Arc_Num = 0;
int space_time_travel_arc = 0;
int input_pa_origin[T_stamp][station] = { 0 };
int input_pa_destin[T_stamp][station] = { 0 };
int scenario_input_pa_origin[T_stamp][station][num_scenario] = { 0 };
int scenario_input_pa_destin[T_stamp][station][num_scenario] = { 0 };

int solution_z[station] = { 0 };
// int solution_z[station]={0};
int solution_z_matrix[max_iteration+1][station] = { 0 };
float Qx_matrix[max_iteration + 1] = { 0 };
float p[num_scenario] = { 0 };
int used_back_up_trains = 0;
int used_depot_trains = 0;
float model_objective = 0;
float lower_bound_second_stage = 0;
float lower_bound_current[max_iteration + 1] = { 0 };
float upper_bound_current[max_iteration + 1] = { 0 };
float Qx = 0;
float objective_relaxed_subproblems[num_scenario] = { 0 };
float objective_original_subproblems[num_scenario] = { 0 };
float passenger_cannot_board[station] = { 0 };
double cutting_list[max_iteration][2] = { 0 };

float passenger_time_board[station][T_stamp][num_scenario] = { 0 };
float passenger_time_alight[station][T_stamp][num_scenario] = { 0 };



float test_objective = 0;
///////////////////////////// Parameters
int* random(int L, int n) {
	static int result[6];
	int sum = 0; // 已生成的随机数总和
	for (int i = 0; i < n - 1; i++) {
		int temp = L - sum;
		int random = rand() % temp;
		result[i] = random;
		sum += random;
	}
	result[n - 1] = L - sum;
	return result;
}
void data_loading()
{

	errno_t input_pa_store;
	input_pa_store = fopen_s(&input_passenger_origin, "D:/Backup_train/oo.txt", "r");
	for (int i = 0; i<T_stamp / 6; i++)
	{
		for (int j = 0; j<station; j++)
		{
			fscanf_s(input_passenger_origin, "%d", &input_pa_origin[i * 6][j]);
		}
	}
	fclose(input_passenger_origin);
	errno_t input_ba_store;
	input_ba_store = fopen_s(&input_passenger_destin, "D:/Backup_train/dd.txt", "r");
	for (int i = 0; i<T_stamp / 6; i++)
	{
		for (int j = 0; j<station; j++)
		{
			fscanf_s(input_passenger_destin, "%d", &input_pa_destin[i * 6][j]);
		}
	}
	fclose(input_passenger_destin);
}
void stochastic_parameter_setting()
{

	for (int i = 0; i<num_scenario; i++)
	{
		p[i] = (float)1 / num_scenario;
	}
	for (int t = 0; t < T_stamp; t++)
	{
		for (int i = 0; i < station; i++)
		{
			input_pa_origin[t][i] = input_pa_origin[t][i] / 2;
			input_pa_destin[t][i] = input_pa_destin[t][i] / 2;
		}
	}


	cout << "OO " << endl; 
	for (int w = 0; w < num_scenario; w++)
	{
		cout << "This is scenario " << w << endl;
		for (int i = 0; i < 120; i++)
		{
			for (int j = 0; j < station; j++)
			{
				int aaa = input_pa_origin[i * 6][j];
				int *p;
				//cout << aaa << ",m ";
				if (aaa > 0)
				{
					p = random(aaa, 6);
					for (int q = 0; q < 6; q++)
					{
						//input_pa_origin[i * 6 + q][j] = p[q];
						scenario_input_pa_origin[i * 6 + q][j][w] = p[q];
					}
				}		
			}
		}
		cout << "DD " << endl;
		for (int i = 0; i < 120; i++)
		{
			for (int j = 0; j < station; j++)
			{
				int aaa = input_pa_destin[i * 6][j];
				int *p;
				if (aaa > 0)
				{
					p = random(aaa, 6);
					for (int q = 0; q < 6; q++)
					{
						//input_pa_destin[i * 6 + q][j] = p[q];
						scenario_input_pa_destin[i * 6 + q][j][w] = p[q];
					}
				}
				//cout << input_pa_destin[i * 6][j] << " ";
			}
			//cout << endl;
		}
	}
	for (int i = 0; i<T_stamp; i++)
	{
		for (int j = 0; j<station; j++)
		{
			for (int w = 0; w<num_scenario; w++)
			{
				if (i == 0)
				{
					passenger_time_board[j][i][w] = origin_total[j][w];
					passenger_time_alight[j][i][w] = destination_total[j][w];
				}
				else
				{
					passenger_time_alight[j][i][w] = passenger_time_alight[j][i - 1][w] + scenario_input_pa_destin[i][j][w];
					passenger_time_board[j][i][w] = scenario_input_pa_origin[i][j][w];
				}
			}
		}
	}
	//for (int i = 0; i<station; i++)
	//{
	//	cout << "Station No. " << i << " ";
	//	for (int t = 0; t < T_stamp; t++)
	//	{
	//		//cout << passenger_time_board[i][t][num_scenario-1] << ", ";
	//		cout << passenger_time_board[i][t][5] << ", ";
	//	}
	//	cout << endl;
	//}
	//cout << endl;
}
// Construct the space time network by connected graph
void Space_Time_Network(int arc_network[][T_stamp*station])
{
	for (int i = 0; i<Space_Time_Node; i++)
	{
		for (int j = 0; j<Space_Time_Node; j++)
		{
			arc_network[i][j] = 0;
		}
	}
	int Arc_index = 1;
	int origin_station, origin_time, destination_station, destination_time;
	for (int i = 0; i<Space_Time_Node; i++)
	{
		origin_station = i / T_stamp;
		origin_time = i%T_stamp;
		for (int j = 0; j<Space_Time_Node; j++)
		{
			destination_station = j / T_stamp;
			destination_time = j%T_stamp;
			if (destination_station - origin_station == 1 && destination_time - origin_time == t_running / t_interval)
			{
				arc_network[i][j] = Arc_index;
				Arc_index++;
			}
			if (destination_station == origin_station && destination_time - origin_time == 1)
			{
				arc_network[i][j] = Arc_index;
				Arc_index++;
			}
		}
	}
	//Space_Time_Arc_Num=3;
}
void g_SaveOutputData(void) 
{
	FILE* g_pFileOriginPAX = NULL;
	g_pFileOriginPAX = fopen("Output_Passenger_Board.csv", "w");
	if (g_pFileOriginPAX == NULL)
	{
		cout << "File OriginPAX.csv cannot be opened." << endl;
	}
	else
	{
		fprintf(g_pFileOriginPAX, "Number of Scenarios = "); fprintf(g_pFileOriginPAX, "%d, ", num_scenario);
		fprintf(g_pFileOriginPAX, "Investment Cost = "); fprintf(g_pFileOriginPAX, "%d, ", investment_cost);
		fprintf(g_pFileOriginPAX, "Number of time units = "); fprintf(g_pFileOriginPAX, "%d, ", T_stamp);
		fprintf(g_pFileOriginPAX, "Instance Type = "); fprintf(g_pFileOriginPAX, "Randomly Generated Instances\n");
		fprintf(g_pFileOriginPAX, "Station No.  , ");
		for (int i = 0; i < station; i++) {
			fprintf(g_pFileOriginPAX, "%d, ", i+1);
		}
		fprintf(g_pFileOriginPAX, "\n");
		for (int w = 0; w < num_scenario; w++)
		{
			fprintf(g_pFileOriginPAX, "Scenario No. = "); fprintf(g_pFileOriginPAX, "%d\n", w);
			for (int t = 0; t < T_stamp; t++)
			{
				fprintf(g_pFileOriginPAX, "%d, ", t);
				for (int i = 0; i < station; i++)
				{
					fprintf(g_pFileOriginPAX, "%.3f, ", passenger_time_board[i][t][w]);
				}
				fprintf(g_pFileOriginPAX, "\n");
			}
		}
	}
	fclose(g_pFileOriginPAX);
	//FILE *g_pFileDestinationPAX = NULL;
	FILE* g_pFileDestinPAX = NULL;
	g_pFileDestinPAX = fopen("Output_Passenger_Alight.csv", "w");
	if (g_pFileDestinPAX == NULL)
	{
		cout << "File OriginPAX.csv cannot be opened." << endl;
	}
	else
	{
		fprintf(g_pFileDestinPAX, "Number of Scenarios = "); fprintf(g_pFileDestinPAX, "%d, ", num_scenario);
		fprintf(g_pFileDestinPAX, "Investment Cost = "); fprintf(g_pFileDestinPAX, "%d, ", investment_cost);
		fprintf(g_pFileDestinPAX, "Number of time units = "); fprintf(g_pFileDestinPAX, "%d, ", T_stamp);
		fprintf(g_pFileDestinPAX, "Instance Type = "); fprintf(g_pFileDestinPAX, "Randomly Generated Instances\n");
		fprintf(g_pFileDestinPAX, "Station No.  , ");
		for (int i = 0; i < station; i++) {
			fprintf(g_pFileDestinPAX, "%d, ", i + 1);
		}
		fprintf(g_pFileDestinPAX, "\n");
		for (int w = 0; w < num_scenario; w++)
		{
			fprintf(g_pFileDestinPAX, "Scenario No. = "); fprintf(g_pFileDestinPAX, "%d\n", w);
			for (int t = 0; t < T_stamp; t++)
			{
				fprintf(g_pFileDestinPAX, "%d, ", t);
				for (int i = 0; i < station; i++)
				{
					fprintf(g_pFileDestinPAX, "%.3f, ", passenger_time_alight[i][t][w]);
				}
				fprintf(g_pFileDestinPAX, "\n");
			}
		}
	}
	fclose(g_pFileDestinPAX);
}



int _tmain()
{
	cout << "Instance yyy: " << num_scenario << endl;
	data_loading();
	clock_t start, finish;
	double totaltime;
	start = clock();
	//srand(unsigned (time(NULL)));
	for (int w = 0; w<num_scenario; w++)
	{
		for (int i = 0; i<station; i++)
		{
			for (int j = 0; j<station; j++)
			{
				if (j>i)
					passenger_od[i][j][w] = rand() % 400;
				else
					passenger_od[i][j][w] = 0;
			}
		}
	}
	float total_demand = 0;
	for (int w = 0; w<num_scenario; w++)
	{
		for (int i = 0; i<station; i++)
		{
			for (int j = 0; j<station; j++)
			{
				origin_total[i][w] += passenger_od[i][j][w];
				destination_total[i][w] += passenger_od[j][i][w];
			}
		}
	}
	// Initialize the parameters of passenger demands
	stochastic_parameter_setting();
	for (int i = 0; i < station; i++)
	{
		for (int w = 0; w < num_scenario; w++)
		{
			for (int t = 0; t < T_stamp; t++)
			{
				total_passenger_demand[w] += passenger_time_board[i][t][w];
			}
		}
	}
	for (int w = 0; w < num_scenario; w++)
	{
		total_demand += total_passenger_demand[w];
	}
	total_demand = total_demand / num_scenario;
	g_SaveOutputData();
	exit(0);
}

