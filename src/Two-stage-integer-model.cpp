// Using "back-up" to reschedule trains 
// CPLEX code and Decomposition approaches 


#include "stdafx.h"
#include <stdio.h>
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
const int Space_Time_Node=T_stamp*station;
typedef struct model_parameters
{
	int qq;
}model_parameters;

int train_esist_position[train_exist]={0, 2, 5, 6, 8, 10, 14, 16, 18, 22};
int total_passenger_demand[num_scenario]={0};
int arc_network[Space_Time_Node][Space_Time_Node]={0};
int passenger_od[station][station][num_scenario];
int index_travel_time_graph[20000][1]={0};
int origin_total[station][num_scenario] = {0};
int destination_total[station][num_scenario]={0};
int Space_Time_Arc_Num=0;
int space_time_travel_arc=0;
int input_pa_origin[T_stamp][station]={0};
int input_pa_destin[T_stamp][station]={0};


int solution_z[station]={0};
// int solution_z[station]={0};
int solution_z_matrix[max_iteration][station]={0};
float Qx_matrix[max_iteration+1]={0};
float p[num_scenario]={0};
int used_back_up_trains=0;
int used_depot_trains=0;
float model_objective=0;
float lower_bound_second_stage=0;
float lower_bound_current[max_iteration+1]={0};
float upper_bound_current[max_iteration+1]={0};
float Qx=0;
float objective_relaxed_subproblems[num_scenario]={0};
float objective_original_subproblems[num_scenario]={0};
float passenger_cannot_board[station]={0};
double cutting_list[max_iteration][2]={0};

float passenger_time_board[station][T_stamp][num_scenario]={0};
float passenger_time_alight[station][T_stamp][num_scenario]={0};



float test_objective = 0;
///////////////////////////// Parameters
void data_loading()
{
	
	errno_t input_pa_store;
	input_pa_store = fopen_s(&input_passenger_origin, "D:/Backup_train/oo.txt","r");
	for(int i=0;i<T_stamp/6; i++)
	{
		for(int j=0;j<station;j++)
		{
			fscanf_s(input_passenger_origin, "%d", &input_pa_origin[i*6][j]);
		}
	}
	fclose (input_passenger_origin);
	errno_t input_ba_store;
	input_ba_store = fopen_s(&input_passenger_destin, "D:/Backup_train/dd.txt","r");
	for(int i=0;i<T_stamp/6; i++)
	{
		for(int j=0;j<station;j++)
		{
			fscanf_s(input_passenger_destin, "%d", &input_pa_destin[i*6][j]);
		}
	}
	fclose (input_passenger_destin);
	
	/*errno_t input_pa_existing;
	input_pa_existing = fopen_s (&input_passenger_existing, "D:/Backup_train/od_matrix.txt","r");
	for(int w=0;w<num_scenario; w++)
	{
		for(int i=0;i<station;i++)
		{
			for(int j=0;j<station;j++)
			{
				fscanf_s(input_passenger_existing, "%d", &passenger_od[i][j][w]);
			}
		}
	}
	fclose (input_passenger_existing);*/
}
void stochastic_parameter_setting()
{
	for (int w = 0; w<num_scenario; w++)
	{
		for (int i = 0; i<station; i++)
		{
			for (int j = 0; j<station; j++)
			{
				if (j>i)
					passenger_od[i][j][w] = rand() % p_volume_para;
				else
					passenger_od[i][j][w] = 0;
			}
		}
	}
	for (int w = 0; w<num_scenario; w++)
	{
		for (int i = 0; i<station; i++)
		{
			for (int j = 0; j<station; j++)
			{
				origin_total[i][w] += passenger_od[i][j][w];
				destination_total[i][w] += passenger_od[j][i][w];
			}
			total_passenger_demand[w] += origin_total[i][w];
		}
	}
	for(int i=0;i<num_scenario;i++)
	{
		p[i]=(float)1/num_scenario;
	}
	//passenger_time_alight[1][42][0] = 20;
	//passenger_time_alight[2][44][0] = 20
//	passenger_time_alight[8][1][0] = 150;
	
//	passenger_time_alight[9][1][0] = 150;

	cout<<"OO "<<endl;
	for(int i=0;i<120; i++)
	{
		for(int j=0;j<station;j++)
		{
			cout<<input_pa_origin[i*6][j]<<" ";
		//	input_pa_origin[i*6][j]=0;
		}
		cout<<endl;
	}	
	cout<<"DD "<<endl;
	for(int i=0;i<120; i++)
	{
		for(int j=0;j<station;j++)
		{
			cout<<input_pa_destin[i*6][j]<<" ";
	//		input_pa_destin[i*6][j]=0;
		}
		cout<<endl;
	}	
	/*input_pa_origin[7][7]=1300;
	input_pa_destin[8][8]=600;
	input_pa_destin[9][9]=700;*/
	for(int i=0;i<T_stamp;i++)
	{
		for(int j=0;j<station;j++)
		{
			for(int w=0;w<num_scenario;w++)
			{
				if(i==0)
				{
					passenger_time_board[j][i][w] = origin_total[j][w];
					passenger_time_alight[j][i][w] = destination_total[j][w];
				}
				else
				{
			//		passenger_time_alight[j][i][w] = passenger_time_alight[j][i-1][w] + passenger_time_alight[j][i][w];
				    passenger_time_alight[j][i][w] = passenger_time_alight[j][i-1][w] + input_pa_destin[i][j];
					passenger_time_board[j][i][w] = input_pa_origin[i][j];
				}
			}
		}
	}
		for(int i=0;i<T_stamp;i++)
		{
			cout<<passenger_time_alight[2][i][0]<<" ";
		}
		cout<<endl;
}
// Construct the space time network by connected graph
void Space_Time_Network(int arc_network[][T_stamp*station])
{
	for(int i=0;i<Space_Time_Node;i++)
	{		
		for(int j=0; j<Space_Time_Node;j++)
		{
			arc_network[i][j]=0;
		}
	}
	int Arc_index=1;
	int origin_station, origin_time, destination_station, destination_time;
	for(int i=0;i<Space_Time_Node;i++)
	{		
		origin_station=i/T_stamp;
		origin_time=i%T_stamp;
		for(int j=0; j<Space_Time_Node;j++)
		{
		destination_station=j/T_stamp;
		destination_time=j%T_stamp;
		if(destination_station-origin_station==1 && destination_time-origin_time==t_running/t_interval)
		{
		  arc_network[i][j]=Arc_index;
		  Arc_index++;
		}
		if(destination_station==origin_station && destination_time-origin_time==1)
		{
		  arc_network[i][j]=Arc_index;
		  Arc_index++;
		}
		}
	}
	//Space_Time_Arc_Num=3;
}
int in_travel_arc_find(int i)
{
	int output;
	if(i-T_stamp-t_running/t_interval>=0)
	{
		output=arc_network[i-T_stamp-t_running/t_interval][i];
	}
	else
	{
		output=0;
	}
	return output;
}
int in_waiting_arc_find(int i)
{
	int output;
	if(i%T_stamp-1>=0)
	{
		output=arc_network[i-1][i];
	//	output=0;
	}
	else
	{
		output=0;
	}
	return output;
}
int out_travel_arc_find(int i)
{
	int output;
	//if(i+T_stamp+t_running/t_interval<=T_stamp-1)
	if((i%T_stamp+t_running/t_interval<=T_stamp-1)&&(i+T_stamp+t_running/t_interval<Space_Time_Node))
	{
		output=arc_network[i][i+T_stamp+t_running/t_interval];
	}
	else
	{
		output=0;
	}
	return output;
}
int out_waiting_arc_find(int i)
{
	int output;
	if(i%T_stamp+1<=T_stamp-1)
	{
		output=arc_network[i][i+1];
	}
	else
	{
		output=0;
	}
	return output;
}
// Use CPLEX TO SOLVE THE ORIGINAL MODEL
void solver_space_time_backup_integer_multi_scenario_new()
{
	int i,j,k,q;
	int in_1, in_2, out_1, out_3; 
	IloEnv env;
	IloNumVarArray backup_train_position(env, station);
	for(i=0;i<station;i++)
	{
		backup_train_position[i]=IloNumVar(env,0, 1, ILOINT);
	}
	NumVarMatrix backuptrain_spatial_dummy_origin(env, num_scenario);
	for(k=0;k<num_scenario;k++)
	{	
		backuptrain_spatial_dummy_origin[k]=IloNumVarArray(env, Space_Time_Node);
		for(i=0;i<Space_Time_Node;i++)
		{
			backuptrain_spatial_dummy_origin[k][i]=IloNumVar(env, 0, 1, ILOINT);
		}
	}
	NumVarMatrix Arc_selection(env, num_scenario);
	NumVarMatrix Passenger_flow_variable(env, num_scenario);
	for(k=0;k<num_scenario;k++)
	{
		Arc_selection[k] = IloNumVarArray(env, Space_Time_Arc_Num+1);
		Passenger_flow_variable[k] = IloNumVarArray(env, Space_Time_Arc_Num+1);
		for(i=0;i<Space_Time_Arc_Num+1;i++)
		{
			Arc_selection[k][i]=IloNumVar(env, 0, 1, ILOINT);
			Passenger_flow_variable[k][i]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
		}
	}
	NumVar3Matrix passenger_boarding(env, num_scenario);
	NumVar3Matrix passenger_alighting(env, num_scenario);
	for(k=0;k<num_scenario;k++)
	{
		passenger_boarding[k]= NumVarMatrix(env, station);
		passenger_alighting[k]= NumVarMatrix(env, station);
		for(i=0;i<station;i++)
		{
			passenger_boarding[k][i]=IloNumVarArray(env, T_stamp);
			passenger_alighting[k][i]=IloNumVarArray(env, T_stamp);
			for(j=0;j<T_stamp;j++)
			{
				passenger_boarding[k][i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
				passenger_alighting[k][i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
			}
		}
	}
	IloModel model(env);
	IloExpr expr(env);
	IloExpr expr_left(env);
	IloExpr num_back_train_position(env);
/*	for(i=0;i<station;i++)
	{
		model.add(backup_train_position[i]==solution_z[i]);
	}*/
	for(k=0;k<num_scenario;k++)
	{
		for(i=0;i<station;i++)
		{
			IloExpr back_flow(env);
			for(j=0;j<T_stamp;j++)
			{
				back_flow += backuptrain_spatial_dummy_origin[k][i*T_stamp+j];
			}
			model.add(back_flow==backup_train_position[i]);
		}
		for(i=0;i<station;i++)
		{
			num_back_train_position += backup_train_position[i];
		}
		model.add(Arc_selection[k][0]==0);
		model.add(Passenger_flow_variable[k][0]==0);
// Flow balance constraints
		IloExpr dummy_destination(env);
		for(i=0;i<Space_Time_Node;i++)
		{
			IloExpr flow_in(env), flow_out(env);
			if(i==0||i==T_stamp*2||i==T_stamp*5||i==T_stamp*6)
			{
				flow_in  = Arc_selection[k][in_travel_arc_find(i)]+Arc_selection[k][in_waiting_arc_find(i)]+1+backuptrain_spatial_dummy_origin[k][i];
				flow_out = Arc_selection[k][out_travel_arc_find(i)]+Arc_selection[k][out_waiting_arc_find(i)];
				model.add(flow_out==flow_in);
			}
		//	else if(i==Space_Time_Node-40||i==Space_Time_Node-30||i==Space_Time_Node-20||i==Space_Time_Node-1)
			else if(i>=(station-1)*T_stamp)
			{
				flow_in  = Arc_selection[k][in_travel_arc_find(i)]+Arc_selection[k][in_waiting_arc_find(i)];
				dummy_destination += flow_in;
			//	flow_out = Arc_selection[w][out_travel_arc_find(i)]+Arc_selection[w][out_waiting_arc_find(i)]+1;
				//model.add(flow_in==flow_out);
			}
			else
			{
				flow_in  = Arc_selection[k][in_travel_arc_find(i)]+Arc_selection[k][in_waiting_arc_find(i)]+backuptrain_spatial_dummy_origin[k][i];
				flow_out = Arc_selection[k][out_travel_arc_find(i)]+Arc_selection[k][out_waiting_arc_find(i)];
				model.add(flow_in==flow_out);
			}	
		}
		model.add(dummy_destination==train_exist+num_back_train_position);
	// Headway constraints
		for(i=0;i<station;i++)
		{
			for(j=h_min-1;j<T_stamp;j++)
			{
				IloExpr flow_in(env);
				for(int t=0;t<h_min;t++)
				{
					flow_in += Arc_selection[k][in_travel_arc_find(i*T_stamp+j-h_min-1+t)];
				}
				model.add(flow_in<=1);
			}
		}
	// Passenger flow constraints
	// Capacity constraints
		for(i=0;i<space_time_travel_arc+1;i++)
		{
			model.add(Passenger_flow_variable[k][index_travel_time_graph[i][0]]<=Arc_selection[k][index_travel_time_graph[i][0]]*train_capacity);
	//model.add(Passenger_flow_variable[index_travel_time_graph[i][0]]<=train_capacity);
		}
		for(i=0;i<station;i++)
		{
			for(j=0;j<T_stamp;j++)
			{
				IloExpr flow_in(env), flow_out(env);
				flow_in  = Passenger_flow_variable[k][in_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[k][in_waiting_arc_find(i*T_stamp+j)];
				flow_out = Passenger_flow_variable[k][out_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[k][out_waiting_arc_find(i*T_stamp+j)];
				if(j==0)
				{
					model.add(flow_out==origin_total[i][k]);
					model.add(passenger_alighting[k][i][j]==0);
				}
				else if(j!=T_stamp-1)
				{
					model.add(passenger_alighting[k][i][j]==flow_in-flow_out);
					model.add(flow_in-flow_out <= Arc_selection[k][in_travel_arc_find(i*T_stamp+j)]*big_M);
				}
			}
		}
		for(i=0;i<station;i++)
		{
			IloExpr total_passenger_origin(env), total_passenger_destination(env);
			for(j=1;j<T_stamp-1;j++)
			{
				total_passenger_destination += passenger_alighting[k][i][j];
			}
			model.add(total_passenger_destination<=destination_total[i][k]);
			expr += (destination_total[i][k] - total_passenger_destination)*T_stamp*p[k];
		}
	}
	// Objective Function
	for(k=0;k<num_scenario;k++)
	{
		for(i=0;i<station;i++)
		{
			for(j=0;j<T_stamp-1;j++)
			{
				expr += j*passenger_alighting[k][i][j]*p[k];
			}
		}
	}
	//expr += expr_left*w_l;
	for(i=0;i<station;i++)
	{
		expr += backup_train_position[i]*investment_cost;
	}
	IloObjective obj = IloMinimize (env, expr);
	model.add(obj);
	// Objective function finished
	IloCplex newcc(model);
	newcc.setParam(IloCplex::TiLim, 600.0);
	newcc.solve();
	cout<<"Solution status = "<<newcc.getCplexStatus()<<endl;
	cout<<"Objective value = "<<newcc.getObjValue()<<endl;
	cout<<"The gap is "<<(newcc.getObjValue()-newcc.getBestObjValue())/(newcc.getObjValue()*100+1)<<endl;
	cout<<"Optimal solution display: "<<endl;
	for(i=0;i<station;i++)
	{
		cout<<newcc.getValue(backup_train_position[i])<<" "<<endl;
	}
	for(i=0;i<station;i++)
	{
		if(i==0)
		{
			used_depot_trains=newcc.getValue(backup_train_position[i]);
		}
		else
		{
		used_back_up_trains += newcc.getValue(backup_train_position[i]);
		}
	}
	// Result display and storage
	model_objective=newcc.getObjValue();
	float ob_1=0, alighting[station]={0};
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			alighting[i] += newcc.getValue(passenger_alighting[0][i][j]); 
		}
		ob_1 += (destination_total[i][0]-alighting[i]);
	}
	cout<<"Objective composition: "<<endl;
	cout<<"Total num passengers:  "<<total_passenger_demand[0]<<endl;
	cout<<"OB1: Detained passenger: "<<ob_1<<endl;
	cout<<"OB2: Travelling time: "<<model_objective-ob_1*w_l<<endl;
    //Data storage
/*	errno_t storage_space_time_flows;
	storage_space_time_flows = fopen_s(&space_time_network_flows, "D:/Backup_train/space_time_network_flows.txt","w");
	int c_a=0;
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_a=newcc.getValue(Arc_selection[k]);
		fprintf(space_time_network_flows, "%d ", c_a);
	}
	fclose(space_time_network_flows);
	errno_t p_flow;
	float c_b=0;
	p_flow = fopen_s(&passenger_flows, "D:/Backup_train/passenger_flows.txt","w");
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_b=newcc.getValue(Passenger_flow_variable[k]);
		fprintf(passenger_flows, "%f ", c_b);
	}
	fclose(passenger_flows);
	
	errno_t storage_alighting_passengers;
	storage_alighting_passengers = fopen_s(&alighting_passengers, "D:/Backup_train/space_time_network_alighting.txt","w");
    float a_flow=0;
	for(i=0; i<station; i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			a_flow=newcc.getValue(passenger_alighting[i][j]);
			fprintf(alighting_passengers, "%f ", a_flow);
		}
		fprintf(alighting_passengers, "\n");
	}
	fclose(alighting_passengers);*/
}  
// Original subproblem: MIP¡¢

// TO BE REVISED HERE
void solver_space_time_backup_integer_single_scenario_revised(int w)
{
	//w=0;
	cout<<"Test "<<origin_total[0][w]<<" "<<origin_total[1][w]<<endl;
	cout<<"Test "<<destination_total[0][w]<<" "<<destination_total[1][w]<<endl;
	int i,j,k,q;
	int in_1, in_2, out_1, out_3; 
	IloEnv env;
	IloNumVarArray backup_train_position(env, station);
	for(i=0;i<station;i++)
	{
		if(i>0)
		{
			backup_train_position[i]=IloNumVar(env,0, 1, ILOINT);
		}
		else
		{
			backup_train_position[i]=IloNumVar(env,0, 1, ILOINT);
		}
	}
	IloNumVarArray backuptrain_spatial_dummy_origin(env, Space_Time_Node);
	for(i=0;i<Space_Time_Node;i++)
	{
		backuptrain_spatial_dummy_origin[i]=IloNumVar(env, 0, 1, ILOINT);
	}
	IloNumVarArray Arc_selection(env, Space_Time_Arc_Num+1);
	IloNumVarArray Passenger_flow_variable(env, Space_Time_Arc_Num+1);
	for(i=0;i<Space_Time_Arc_Num+1;i++)
	{
		Arc_selection[i]=IloNumVar(env, 0, 1, ILOINT);
		Passenger_flow_variable[i]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
	}
	NumVarMatrix passenger_boarding(env, station);
	NumVarMatrix passenger_alighting(env, station);
	for(i=0;i<station;i++)
	{
		passenger_boarding[i]=IloNumVarArray(env, T_stamp);
		passenger_alighting[i]=IloNumVarArray(env, T_stamp);
		for(j=0;j<T_stamp;j++)
		{
			passenger_boarding[i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
			passenger_alighting[i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
		}
	}
	IloModel model(env);
	IloExpr expr(env);
	IloExpr expr_left(env);
	IloExpr num_back_train_position(env);
	
	for(i=0;i<station;i++)
	{
		model.add(backup_train_position[i]==solution_z[i]);
	}
	
for(i=0;i<station;i++)
	{
		IloExpr back_flow(env);
		for(j=0;j<T_stamp;j++)
		{
			back_flow += backuptrain_spatial_dummy_origin[i*T_stamp+j];
		}
		model.add(back_flow==backup_train_position[i]);
	}
	for(i=0;i<station;i++)
	{
		num_back_train_position += backup_train_position[i];
	}
	model.add(Arc_selection[0]==0);
	model.add(Passenger_flow_variable[0]==0);
	
// Flow balance constraints
	
	IloExpr dummy_destination(env);
	for(i=0;i<Space_Time_Node;i++)
	{
		IloExpr flow_in(env), flow_out(env);
		if(i==0||i==T_stamp*2||i==T_stamp*5||i==T_stamp*6)
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)]+1+backuptrain_spatial_dummy_origin[i];
			flow_out = Arc_selection[out_travel_arc_find(i)]+Arc_selection[out_waiting_arc_find(i)];
			model.add(flow_out==flow_in);
		}
		//	else if(i==Space_Time_Node-40||i==Space_Time_Node-30||i==Space_Time_Node-20||i==Space_Time_Node-1)
		else if(i>=(station-1)*T_stamp)
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)];
			dummy_destination += flow_in;
			//	flow_out = Arc_selection[w][out_travel_arc_find(i)]+Arc_selection[w][out_waiting_arc_find(i)]+1;
				//model.add(flow_in==flow_out);
		}
		else
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)]+backuptrain_spatial_dummy_origin[i];
			flow_out = Arc_selection[out_travel_arc_find(i)]+Arc_selection[out_waiting_arc_find(i)];
			model.add(flow_in==flow_out);
		}	
	}
	model.add(dummy_destination == train_exist + num_back_train_position);
	// Headway constraints
	
	for(i=0;i<station;i++)
	{
		for(j=h_min-1;j<T_stamp;j++)
		{
			IloExpr flow_in(env);
			for(int t=0;t<h_min;t++)
			{
				flow_in += Arc_selection[in_travel_arc_find(i*T_stamp+j-h_min-1+t)];
			}
			model.add(flow_in<=1);
		}
	}
	// Passenger flow constraints
	// Capacity constraints
	
	for(i=0;i<space_time_travel_arc+1;i++)
	{
	model.add(Passenger_flow_variable[index_travel_time_graph[i][0]]<=Arc_selection[index_travel_time_graph[i][0]]*train_capacity);
	//model.add(Passenger_flow_variable[index_travel_time_graph[i][0]]<=train_capacity);
	}
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp;j++)
		{
			IloExpr flow_in(env), flow_out(env);
			flow_in  = Passenger_flow_variable[in_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[in_waiting_arc_find(i*T_stamp+j)];
			flow_out = Passenger_flow_variable[out_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[out_waiting_arc_find(i*T_stamp+j)];
			if(j==0)
			{
				model.add(flow_out==origin_total[i][w]);
				//model.add(flow_out==800);
				model.add(passenger_alighting[i][j]==0);
			}
			else if(j!=T_stamp-1)
				{
				model.add(passenger_alighting[i][j]==flow_in-flow_out);
				model.add(flow_in-flow_out <= Arc_selection[in_travel_arc_find(i*T_stamp+j)]*big_M);
			}
		}
	}
	for(i=0;i<station;i++)
	{
		IloExpr total_passenger_origin(env), total_passenger_destination(env);
		for(j=1;j<T_stamp-1;j++)
		{
			total_passenger_destination += passenger_alighting[i][j];
		}
		 model.add(total_passenger_destination<=destination_total[i][w]);
		expr += (destination_total[i][w] - total_passenger_destination)*T_stamp;
	}
	
	// Objective Function
	
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			expr += j*passenger_alighting[i][j];
		}
	}
	//expr += expr_left*w_l;
	for(i=0;i<station;i++)
	{
		expr += backup_train_position[i]*investment_cost;
	}
	

	IloObjective obj = IloMinimize (env, expr);
	model.add(obj);
	// Objective function finished
	IloCplex newcc(model);
	newcc.setParam(IloCplex::TiLim, 150.0);
	newcc.solve();
	cout<<"Solution status = "<<newcc.getCplexStatus()<<endl;
	cout<<"Objective value = "<<newcc.getObjValue()<<endl;
	cout<<"The gap is "<<(newcc.getObjValue()-newcc.getBestObjValue())/(newcc.getObjValue()*100+1)<<endl;
	/*
	for(i=0;i<station;i++)
	{
		if(i==0)
		{
			used_depot_trains=newcc.getValue(backup_train_position[i]);
		}
		else
		{
		used_back_up_trains += newcc.getValue(backup_train_position[i]);
		}
	}
	// Result display and storage
	model_objective=newcc.getObjValue();
	objective_original_subproblems[w]=model_objective;
	float ob_1=0, alighting[station]={0};
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			alighting[i] += newcc.getValue(passenger_alighting[i][j]); 
		}
		ob_1 += (destination_total[i][w]-alighting[i]);
	}
	cout<<"Objective composition: "<<endl;
	cout<<"Total num passengers:  "<<total_passenger_demand[w]<<endl;
	cout<<"OB1: Detained passenger: "<<ob_1<<endl;
	cout<<"OB2: Travelling time: "<<model_objective-ob_1*w_l<<endl;
	
    //Data storage
	/*
	errno_t storage_space_time_flows;
	storage_space_time_flows = fopen_s(&space_time_network_flows, "D:/Backup_train/space_time_network_flows.txt","w");
	int c_a=0;
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_a=newcc.getValue(Arc_selection[k]);
		fprintf(space_time_network_flows, "%d ", c_a);
	}
	fclose(space_time_network_flows);
	errno_t p_flow;
	float c_b=0;
	p_flow = fopen_s(&passenger_flows, "D:/Backup_train/passenger_flows.txt","w");
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_b=newcc.getValue(Passenger_flow_variable[k]);
		fprintf(passenger_flows, "%f ", c_b);
	}
	fclose(passenger_flows);
	
	errno_t storage_alighting_passengers;
	storage_alighting_passengers = fopen_s(&alighting_passengers, "D:/Backup_train/space_time_network_alighting.txt","w");
    float a_flow=0;
	for(i=0; i<station; i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			a_flow=newcc.getValue(passenger_alighting[i][j]);
			fprintf(alighting_passengers, "%f ", a_flow);
		}
		fprintf(alighting_passengers, "\n");
	}
	fclose(alighting_passengers);*/
}  
// Relaxed subproblem: LP
void solver_space_time_backup_linear_single_scenario_revised(int w)
{
	int i,j,k,q;
	int in_1, in_2, out_1, out_3; 
	IloEnv env;
	IloNumVarArray backup_train_position(env, station);
	for(i=0;i<station;i++)
	{
		if(i>0)
		{
			backup_train_position[i]=IloNumVar(env,0, 1, ILOFLOAT);
		}
		else
		{
			backup_train_position[i]=IloNumVar(env,0, 1, ILOFLOAT);
		}
	}
	IloNumVarArray backuptrain_spatial_dummy_origin(env, Space_Time_Node);
	for(i=0;i<Space_Time_Node;i++)
	{
		backuptrain_spatial_dummy_origin[i]=IloNumVar(env, 0, 1, ILOFLOAT);
	}
	IloNumVarArray Arc_selection(env, Space_Time_Arc_Num+1);
	IloNumVarArray Passenger_flow_variable(env, Space_Time_Arc_Num+1);
	for(i=0;i<Space_Time_Arc_Num+1;i++)
	{
		Arc_selection[i]=IloNumVar(env, 0, 1, ILOFLOAT);
		Passenger_flow_variable[i]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
	}
	NumVarMatrix passenger_boarding(env, station);
	NumVarMatrix passenger_alighting(env, station);
	for(i=0;i<station;i++)
	{
		passenger_boarding[i]=IloNumVarArray(env, T_stamp);
		passenger_alighting[i]=IloNumVarArray(env, T_stamp);
		for(j=0;j<T_stamp;j++)
		{
			passenger_boarding[i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
			passenger_alighting[i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
		}
	}
	IloModel model(env);
	IloExpr expr(env);
	IloExpr expr_left(env);
	IloExpr num_back_train_position(env);
	for(i=0;i<station;i++)
	{
		model.add(backup_train_position[i]==solution_z[i]);
	}
	
for(i=0;i<station;i++)
	{
		IloExpr back_flow(env);
		for(j=0;j<T_stamp;j++)
		{
			back_flow += backuptrain_spatial_dummy_origin[i*T_stamp+j];
		}
		model.add(back_flow==backup_train_position[i]);
	}
	for(i=0;i<station;i++)
	{
		num_back_train_position += backup_train_position[i];
	}
	model.add(Arc_selection[0]==0);
	model.add(Passenger_flow_variable[0]==0);
// Flow balance constraints
	IloExpr dummy_destination(env);
	for(i=0;i<Space_Time_Node;i++)
	{
		IloExpr flow_in(env), flow_out(env);
		if(i==0||i==T_stamp*2||i==T_stamp*5||i==T_stamp*6)
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)]+1+backuptrain_spatial_dummy_origin[i];
			flow_out = Arc_selection[out_travel_arc_find(i)]+Arc_selection[out_waiting_arc_find(i)];
			model.add(flow_out==flow_in);
		}
		//	else if(i==Space_Time_Node-40||i==Space_Time_Node-30||i==Space_Time_Node-20||i==Space_Time_Node-1)
		else if(i>=(station-1)*T_stamp)
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)];
			dummy_destination += flow_in;
			//	flow_out = Arc_selection[w][out_travel_arc_find(i)]+Arc_selection[w][out_waiting_arc_find(i)]+1;
				//model.add(flow_in==flow_out);
		}
		else
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)]+backuptrain_spatial_dummy_origin[i];
			flow_out = Arc_selection[out_travel_arc_find(i)]+Arc_selection[out_waiting_arc_find(i)];
			model.add(flow_in==flow_out);
		}	
	}
	model.add(dummy_destination==train_exist+num_back_train_position);
	// Headway constraints
	for(i=0;i<station;i++)
	{
		for(j=h_min-1;j<T_stamp;j++)
		{
			IloExpr flow_in(env);
			for(int t=0;t<h_min;t++)
			{
				flow_in += Arc_selection[in_travel_arc_find(i*T_stamp+j-h_min-1+t)];
			}
			model.add(flow_in<=1);
		}
	}
	// Passenger flow constraints
	// Capacity constraints
	for(i=0;i<space_time_travel_arc+1;i++)
	{
	model.add(Passenger_flow_variable[index_travel_time_graph[i][0]]<=Arc_selection[index_travel_time_graph[i][0]]*train_capacity);
	//model.add(Passenger_flow_variable[index_travel_time_graph[i][0]]<=train_capacity);
	}
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp;j++)
		{
			IloExpr flow_in(env), flow_out(env);
			flow_in  = Passenger_flow_variable[in_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[in_waiting_arc_find(i*T_stamp+j)];
			flow_out = Passenger_flow_variable[out_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[out_waiting_arc_find(i*T_stamp+j)];
			if(j==0)
			{
				model.add(flow_out==origin_total[i][w]);
				model.add(passenger_alighting[i][j]==0);
			}
			else if(j!=T_stamp-1)
				{
				model.add(passenger_alighting[i][j]==flow_in-flow_out);
				model.add(flow_in-flow_out <= Arc_selection[in_travel_arc_find(i*T_stamp+j)]*big_M);
			}
		}
	}
	for(i=0;i<station;i++)
	{
		IloExpr total_passenger_origin(env), total_passenger_destination(env);
		for(j=1;j<T_stamp-1;j++)
		{
			total_passenger_destination += passenger_alighting[i][j];
		}
		model.add(total_passenger_destination<=destination_total[i][w]);
		expr += (destination_total[i][w] - total_passenger_destination)*T_stamp;
	}
	// Objective Function
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			expr += j*passenger_alighting[i][j];
		}
	}
	//expr += expr_left*w_l;
	/*for(i=0;i<station;i++)
	{
		if(i==0)
		{
			expr += backup_train_position[i]*investment_cost_depot;
		}
		else
		{
		expr += backup_train_position[i]*investment_cost;
		}
	}*/
	IloObjective obj = IloMinimize (env, expr);
	model.add(obj);
	// Objective function finished
	IloCplex newcc(model);
	newcc.setParam(IloCplex::TiLim, 150.0);
	newcc.solve();
	cout<<"Solution status = "<<newcc.getCplexStatus()<<endl;
	cout<<"Objective value = "<<newcc.getObjValue()<<endl;
	cout<<"The gap is "<<(newcc.getObjValue()-newcc.getBestObjValue())/(newcc.getObjValue()*100+1)<<endl;
	for(i=0;i<station;i++)
	{
		if(i==0)
		{
			used_depot_trains=newcc.getValue(backup_train_position[i]);
		}
		else
		{
		used_back_up_trains += newcc.getValue(backup_train_position[i]);
		}
	}
	// Result display and storage
	model_objective=newcc.getObjValue();
	objective_original_subproblems[w]=model_objective;
	float ob_1=0, alighting[station]={0};
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			alighting[i] += newcc.getValue(passenger_alighting[i][j]); 
		}
		ob_1 += (destination_total[i][w]-alighting[i]);
	}
	cout<<"Objective composition: "<<endl;
	cout<<"Total num passengers:  "<<total_passenger_demand[w]<<endl;
	cout<<"OB1: Detained passenger: "<<ob_1<<endl;
	cout<<"OB2: Travelling time: "<<model_objective-ob_1*w_l<<endl;
    //Data storage
	errno_t storage_space_time_flows;
	storage_space_time_flows = fopen_s(&space_time_network_flows, "D:/Backup_train/space_time_network_flows.txt","w");
	float c_a=0;
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_a=newcc.getValue(Arc_selection[k]);
		fprintf(space_time_network_flows, "%f ", c_a);
	}
	fclose(space_time_network_flows);
	errno_t p_flow;
	float c_b=0;
	p_flow = fopen_s(&passenger_flows, "D:/Backup_train/passenger_flows.txt","w");
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_b=newcc.getValue(Passenger_flow_variable[k]);
		fprintf(passenger_flows, "%f ", c_b);
	}
	fclose(passenger_flows);
	
	errno_t storage_alighting_passengers;
	storage_alighting_passengers = fopen_s(&alighting_passengers, "D:/Backup_train/space_time_network_alighting.txt","w");
    float a_flow=0;
	for(i=0; i<station; i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			a_flow=newcc.getValue(passenger_alighting[i][j]);
			fprintf(alighting_passengers, "%f ", a_flow);
		}
		fprintf(alighting_passengers, "\n");
	}
	fclose(alighting_passengers);
}  

void time_dependent_solver_space_time_backup_linear_single_scenario_revised(int w)
{
	int i,j,k,q;
	int in_1, in_2, out_1, out_3; 
	IloEnv env;
	IloNumVarArray backup_train_position(env, station);
	for(i=0;i<station;i++)
	{
		if(i>0)
		{
			backup_train_position[i]=IloNumVar(env,0, 1, ILOFLOAT);
		}
		else
		{
			backup_train_position[i]=IloNumVar(env,0, 1, ILOFLOAT);
		}
	}
	IloNumVarArray backuptrain_spatial_dummy_origin(env, Space_Time_Node);
	for(i=0;i<Space_Time_Node;i++)
	{
		backuptrain_spatial_dummy_origin[i]=IloNumVar(env, 0, 1, ILOFLOAT);
	}
	IloNumVarArray Arc_selection(env, Space_Time_Arc_Num+1);
	IloNumVarArray Passenger_flow_variable(env, Space_Time_Arc_Num+1);
	for(i=0;i<Space_Time_Arc_Num+1;i++)
	{
		Arc_selection[i]=IloNumVar(env, 0, 1, ILOFLOAT);
		Passenger_flow_variable[i]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
	}
	NumVarMatrix passenger_boarding(env, station);
	NumVarMatrix passenger_alighting(env, station);
	for(i=0;i<station;i++)
	{
		passenger_boarding[i]=IloNumVarArray(env, T_stamp);
		passenger_alighting[i]=IloNumVarArray(env, T_stamp);
		for(j=0;j<T_stamp;j++)
		{
			passenger_boarding[i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
			passenger_alighting[i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
		}
	}
	IloModel model(env);
	IloExpr expr(env);
	IloExpr expr_left(env);
	IloExpr num_back_train_position(env);
	for(i=0;i<station;i++)
	{
		model.add(backup_train_position[i]==solution_z[i]);
	}
	
for(i=0;i<station;i++)
	{
		IloExpr back_flow(env);
		for(j=0;j<T_stamp;j++)
		{
			back_flow += backuptrain_spatial_dummy_origin[i*T_stamp+j];
		}
		model.add(back_flow==backup_train_position[i]);
	}
	for(i=0;i<station;i++)
	{
		num_back_train_position += backup_train_position[i];
	}
	model.add(Arc_selection[0]==0);
	model.add(Passenger_flow_variable[0]==0);
// Flow balance constraints
	IloExpr dummy_destination(env);
	for(i=0;i<Space_Time_Node;i++)
	{
		IloExpr flow_in(env), flow_out(env);
		if(i==0||i==T_stamp*2||i==T_stamp*5||i==T_stamp*6)
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)]+1+backuptrain_spatial_dummy_origin[i];
			flow_out = Arc_selection[out_travel_arc_find(i)]+Arc_selection[out_waiting_arc_find(i)];
			model.add(flow_out==flow_in);
		}
		//	else if(i==Space_Time_Node-40||i==Space_Time_Node-30||i==Space_Time_Node-20||i==Space_Time_Node-1)
		else if(i>=(station-1)*T_stamp)
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)];
			dummy_destination += flow_in;
			//	flow_out = Arc_selection[w][out_travel_arc_find(i)]+Arc_selection[w][out_waiting_arc_find(i)]+1;
				//model.add(flow_in==flow_out);
		}
		else
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)]+backuptrain_spatial_dummy_origin[i];
			flow_out = Arc_selection[out_travel_arc_find(i)]+Arc_selection[out_waiting_arc_find(i)];
			model.add(flow_in==flow_out);
		}	
	}
	model.add(dummy_destination==train_exist+num_back_train_position);
	// Headway constraints
	for(i=0;i<station;i++)
	{
		for(j=h_min-1;j<T_stamp;j++)
		{
			IloExpr flow_in(env);
			for(int t=0;t<h_min;t++)
			{
				flow_in += Arc_selection[in_travel_arc_find(i*T_stamp+j-h_min-1+t)];
			}
			model.add(flow_in<=1);
		}
	}
	// Passenger flow constraints
	// Capacity constraints
	for(i=0;i<space_time_travel_arc+1;i++)
	{
	model.add(Passenger_flow_variable[index_travel_time_graph[i][0]]<=Arc_selection[index_travel_time_graph[i][0]]*train_capacity);
	//model.add(Passenger_flow_variable[index_travel_time_graph[i][0]]<=train_capacity);
	}
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp;j++)
		{
			IloExpr flow_in(env), flow_out(env);
			flow_in  = Passenger_flow_variable[in_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[in_waiting_arc_find(i*T_stamp+j)];
			flow_out = Passenger_flow_variable[out_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[out_waiting_arc_find(i*T_stamp+j)];
			if(j==0)
			{
				model.add(flow_out==origin_total[i][w]);
				model.add(passenger_alighting[i][j]==0);
			}
			else if(j!=T_stamp-1)
				{
				model.add(passenger_alighting[i][j] == passenger_time_board[i][j][w] + flow_in-flow_out);
				model.add(passenger_alighting[i][j] <= Arc_selection[in_travel_arc_find(i*T_stamp+j)]*big_M);
			}
		}
	}
	
	/*for(i=0;i<station;i++)
	{
		IloExpr total_passenger_origin(env), total_passenger_destination(env);
		for(j=1;j<T_stamp-1;j++)
		{
			total_passenger_destination += passenger_alighting[i][j];
		}
		model.add(total_passenger_destination<=destination_total[i][w]);
		expr += (destination_total[i][w] - total_passenger_destination)*T_stamp;
	} */
	
	
	for(i=0; i<station; i++)
	{
		IloExpr total_passenger_destination(env);
		for(j=1; j<T_stamp-1; j++)
		{
			IloExpr total_passenger_destination_current(env);
			for(int t=0; t<j+1; t++)
			{
				total_passenger_destination_current += passenger_alighting[i][t]; 
			}
			model.add(total_passenger_destination_current<= passenger_time_alight[i][j][w]);
	/*	if(j==T_stamp-2)
			{
				expr += (passenger_time_alight[i][j][w]-total_passenger_destination_current)*T_stamp;
			}*/
			total_passenger_destination += passenger_alighting[i][j];
		}
		expr += (passenger_time_alight[i][T_stamp-2][w] - total_passenger_destination)*T_stamp;
		//T_stamp
	}
	// Objective Function
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			expr += j*passenger_alighting[i][j];
			//expr += j*passenger_alighting[i][j];
		}
	}
	//expr += expr_left*w_l;
	/*for(i=0;i<station;i++)
	{
		if(i==0)
		{
			expr += backup_train_position[i]*investment_cost_depot;
		}
		else
		{
		expr += backup_train_position[i]*investment_cost;
		}
	}*/
	IloObjective obj = IloMinimize (env, expr);
	model.add(obj);
	// Objective function finished
	IloCplex newcc(model);
	newcc.setParam(IloCplex::TiLim, 150.0);
	newcc.solve();
	cout<<"Solution status = "<<newcc.getCplexStatus()<<endl;
	cout<<"Objective value = "<<newcc.getObjValue()<<endl;
	cout<<"The gap is "<<(newcc.getObjValue()-newcc.getBestObjValue())/(newcc.getObjValue()*100+1)<<endl;
	for(i=0;i<station;i++)
	{
		if(i==0)
		{
			used_depot_trains=newcc.getValue(backup_train_position[i]);
		}
		else
		{
		used_back_up_trains += newcc.getValue(backup_train_position[i]);
		}
	}
	// Result display and storage
	model_objective=newcc.getObjValue();
	objective_original_subproblems[w]=model_objective;
	float ob_1=0, alighting[station]={0};
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			alighting[i] += newcc.getValue(passenger_alighting[i][j]); 
		}
		ob_1 += (passenger_time_alight[i][T_stamp-2][w]-alighting[i]);
	}
	cout<<"Objective composition: "<<endl;
	cout<<"Total num passengers:  "<<total_passenger_demand[w]<<endl;
	cout<<"OB1: Detained passenger: "<<ob_1<<endl;
	cout<<"OB2: Travelling time: "<<model_objective-ob_1*w_l<<endl;
    //Data storage
	errno_t storage_space_time_flows;
	storage_space_time_flows = fopen_s(&space_time_network_flows, "D:/Backup_train/space_time_network_flows.txt","w");
	int c_a=0;
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_a=newcc.getValue(Arc_selection[k]);
		fprintf(space_time_network_flows, "%d ", c_a);
	}
	fclose(space_time_network_flows);
	errno_t p_flow;
	float c_b=0;
	p_flow = fopen_s(&passenger_flows, "D:/Backup_train/passenger_flows.txt","w");
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_b=newcc.getValue(Passenger_flow_variable[k]);
		fprintf(passenger_flows, "%f ", c_b);
	}
	fclose(passenger_flows);
	
	errno_t storage_alighting_passengers;
	storage_alighting_passengers = fopen_s(&alighting_passengers, "D:/Backup_train/space_time_network_alighting.txt","w");
    float a_flow=0;
	for(i=0; i<station; i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			a_flow=newcc.getValue(passenger_alighting[i][j]);
			fprintf(alighting_passengers, "%f ", a_flow);
		}
		fprintf(alighting_passengers, "\n");
	}
	fclose(alighting_passengers);
}  


// FLOAT VERSION
float integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_1(int w)
{
	int i,j,k,q;
	int in_1, in_2, out_1, out_3; 
	IloEnv env;
	IloNumVarArray backup_train_position(env, station);
	for(i=0;i<station;i++)
	{
		if(i>0)
		{
			backup_train_position[i]=IloNumVar(env,0, 1, ILOFLOAT);
		}
		else
		{
			backup_train_position[i]=IloNumVar(env,0, 1, ILOFLOAT);
		}
	}
	IloNumVarArray backuptrain_spatial_dummy_origin(env, Space_Time_Node);
	for(i=0;i<Space_Time_Node;i++)
	{
		backuptrain_spatial_dummy_origin[i]=IloNumVar(env, 0, 1, ILOFLOAT);
	}
	IloNumVarArray Arc_selection(env, Space_Time_Arc_Num+1);
	IloNumVarArray Passenger_flow_variable(env, Space_Time_Arc_Num+1);
	for(i=0;i<Space_Time_Arc_Num+1;i++)
	{
		Arc_selection[i]=IloNumVar(env, 0, 1, ILOFLOAT);
		Passenger_flow_variable[i]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
	}
	NumVarMatrix passenger_boarding(env, station);
	NumVarMatrix passenger_alighting(env, station);
	for(i=0;i<station;i++)
	{
		passenger_boarding[i]=IloNumVarArray(env, T_stamp);
		passenger_alighting[i]=IloNumVarArray(env, T_stamp);
		for(j=0;j<T_stamp;j++)
		{
			passenger_boarding[i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
			passenger_alighting[i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
		}
	}
	IloModel model(env);
	IloExpr expr(env);
	IloExpr expr_left(env);
	IloExpr num_back_train_position(env);
	for(i=0;i<station;i++)
	{
		model.add(backup_train_position[i]==solution_z[i]);
	}
	
for(i=0;i<station;i++)
	{
		IloExpr back_flow(env);
		for(j=0;j<T_stamp;j++)
		{
			back_flow += backuptrain_spatial_dummy_origin[i*T_stamp+j];
		}
		model.add(back_flow==backup_train_position[i]);
	}
	for(i=0;i<station;i++)
	{
		num_back_train_position += backup_train_position[i];
	}
	model.add(Arc_selection[0]==0);
	model.add(Passenger_flow_variable[0]==0);
// Flow balance constraints
	IloExpr dummy_destination(env);
	for(i=0;i<Space_Time_Node;i++)
	{
		IloExpr flow_in(env), flow_out(env);
		if(find(train_esist_position, i)==1)
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)]+1+backuptrain_spatial_dummy_origin[i];
			flow_out = Arc_selection[out_travel_arc_find(i)]+Arc_selection[out_waiting_arc_find(i)];
			model.add(flow_out==flow_in);
		}
		else if(i>=(station-1)*T_stamp)
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)];
			dummy_destination += flow_in;
			//	flow_out = Arc_selection[w][out_travel_arc_find(i)]+Arc_selection[w][out_waiting_arc_find(i)]+1;
				//model.add(flow_in==flow_out);
		}
		else
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)]+backuptrain_spatial_dummy_origin[i];
			flow_out = Arc_selection[out_travel_arc_find(i)]+Arc_selection[out_waiting_arc_find(i)];
			model.add(flow_in==flow_out);
		}	
	}
	model.add(dummy_destination==train_exist+num_back_train_position);
	// Headway constraints
	for(i=0;i<station;i++)
	{
		for(j=h_min-1;j<T_stamp;j++)
		{
			IloExpr flow_in(env);
			for(int t=0;t<h_min;t++)
			{
				flow_in += Arc_selection[in_travel_arc_find(i*T_stamp+j-h_min-1+t)];
			}
			model.add(flow_in<=1);
		}
	}
	for(i=0;i<space_time_travel_arc+1;i++)
	{
		model.add(Passenger_flow_variable[index_travel_time_graph[i][0]] <= Arc_selection[index_travel_time_graph[i][0]]*train_capacity);
	}
		for(i=0;i<station;i++)
		{
			for(j=0;j<T_stamp;j++)
			{
				IloExpr flow_in(env), flow_out(env);
				flow_in  = Passenger_flow_variable[in_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[in_waiting_arc_find(i*T_stamp+j)];
				flow_out = Passenger_flow_variable[out_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[out_waiting_arc_find(i*T_stamp+j)];
				if(j==0)
				{
					model.add(flow_out==origin_total[i][w]);
					model.add(passenger_alighting[i][j]==0);
				}
				else if(j!=T_stamp-1)
				{
					model.add(passenger_alighting[i][j] == passenger_time_board[i][j][w] + flow_in-flow_out);
					model.add(passenger_alighting[i][j] <= Arc_selection[in_travel_arc_find(i*T_stamp+j)]*big_M);
				}
			}
		}
		for(i=0; i<station; i++)
		{
			IloExpr total_passenger_destination(env);
			for(j=1; j<T_stamp-1; j++)
			{
				IloExpr total_passenger_destination_current(env);
				for(int t=0; t<j+1; t++)
				{
					total_passenger_destination_current += passenger_alighting[i][t]; 
				}
				model.add(total_passenger_destination_current<= passenger_time_alight[i][j][w]);
				total_passenger_destination += passenger_alighting[i][j];
			}
			expr += (passenger_time_alight[i][T_stamp-2][w] - total_passenger_destination)*T_stamp*penlty;
		}
	// Objective Function
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			expr += j*passenger_alighting[i][j];
		}
	}
	//expr += expr_left*w_l;
	/*for(i=0;i<station;i++)
	{
		expr += backup_train_position[i]*investment_cost;
	}*/
	// Objective Function
	IloObjective obj = IloMinimize (env, expr);
	model.add(obj);
	// Objective function finished
	IloCplex newcc(model);
	newcc.setParam(IloCplex::TiLim, 3000.0);
	newcc.setOut(env.getNullStream());
	// newcc.setParam(IloCplex::RootAlg,1);
	//newcc.solve(IloCplex::Algorithm::Barrier);
	newcc.solve();
	//newcc.setParam(IloCplex::Algorithm::Barrier);
	//newcc.setParam(IloCplex::Algorithm::Barrier);
	cout<<"Solution status = "<<newcc.getCplexStatus()<<endl;
	cout<<"Objective value = "<<newcc.getObjValue()<<endl;
	test_objective = newcc.getObjValue();
	cout<<"The gap is "<<(newcc.getObjValue()-newcc.getBestObjValue())/(newcc.getObjValue()*100+1)<<endl;
/*	float judge;
	int xq=0;
	for(i=0;i<Space_Time_Arc_Num+1;i++)
	{
		judge = newcc.getValue(Arc_selection[i]);
		if(judge - (int)judge != 0)
		{
			cout<<judge<<" "<<endl;
		}
	}*/
/*	cout<<"This is an integer solution? : = "<<xq<<endl;
	for(i=0;i<station;i++)
	{
		if(i==0)
		{
			used_depot_trains=newcc.getValue(backup_train_position[i]);
		}
		else
		{
		used_back_up_trains += newcc.getValue(backup_train_position[i]);
		}
	}*/
	// Result display and storage
	model_objective=newcc.getObjValue();
	objective_original_subproblems[w]=model_objective;
	float ob_1=0, alighting[station]={0};
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			alighting[i] += newcc.getValue(passenger_alighting[i][j]); 
		}
		ob_1 += (passenger_time_alight[i][T_stamp-2][w]-alighting[i]);
	}
	cout<<"Objective composition: "<<endl;
	cout<<"Total num passengers:  "<<total_passenger_demand[w]<<endl;
	cout<<"OB1: Detained passenger: "<<ob_1<<endl;
	cout<<"OB2: Travelling time: "<<model_objective-ob_1*w_l<<endl;
    //Data storage
	/*
	errno_t storage_space_time_flows;
	storage_space_time_flows = fopen_s(&space_time_network_flows, "D:/Backup_train/space_time_network_flows.txt","w");
	int c_a=0;
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_a=newcc.getValue(Arc_selection[k]);
		fprintf(space_time_network_flows, "%d ", c_a);
	}
	fclose(space_time_network_flows);
	errno_t p_flow;
	float c_b=0;
	p_flow = fopen_s(&passenger_flows, "D:/Backup_train/passenger_flows.txt","w");
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_b=newcc.getValue(Passenger_flow_variable[k]);
		fprintf(passenger_flows, "%f ", c_b);
	}
	fclose(passenger_flows);
	
	errno_t storage_alighting_passengers;
	storage_alighting_passengers = fopen_s(&alighting_passengers, "D:/Backup_train/space_time_network_alighting.txt","w");
    float a_flow=0;
	for(i=0; i<station; i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			a_flow=newcc.getValue(passenger_alighting[i][j]);
			fprintf(alighting_passengers, "%f ", a_flow);
		}
		fprintf(alighting_passengers, "\n");
	}
	fclose(alighting_passengers);*/
	newcc.end();
	env.end();
	return model_objective;
}  

// FLOAT VERSION
void integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_2(int w)
{
	int i,j,k,q;
	int in_1, in_2, out_1, out_3; 
	IloEnv env;
	IloNumVarArray backup_train_position(env, station);
	for(i=0;i<station;i++)
	{
		if(i>0)
		{
			backup_train_position[i]=IloNumVar(env,0, 1, ILOFLOAT);
		}
		else
		{
			backup_train_position[i]=IloNumVar(env,0, 1, ILOFLOAT);
		}
	}
	IloNumVarArray backuptrain_spatial_dummy_origin(env, Space_Time_Node);
	for(i=0;i<Space_Time_Node;i++)
	{
		backuptrain_spatial_dummy_origin[i]=IloNumVar(env, 0, 1, ILOFLOAT);
	}
	IloNumVarArray Arc_selection(env, Space_Time_Arc_Num+1);
	IloNumVarArray Passenger_flow_variable(env, Space_Time_Arc_Num+1);
	for(i=0;i<Space_Time_Arc_Num+1;i++)
	{
		Arc_selection[i]=IloNumVar(env, 0, 1, ILOINT);
		Passenger_flow_variable[i]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
	}
	NumVarMatrix passenger_boarding(env, station);
	NumVarMatrix passenger_alighting(env, station);
	for(i=0;i<station;i++)
	{
		passenger_boarding[i]=IloNumVarArray(env, T_stamp);
		passenger_alighting[i]=IloNumVarArray(env, T_stamp);
		for(j=0;j<T_stamp;j++)
		{
			passenger_boarding[i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
			passenger_alighting[i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
		}
	}
	IloModel model(env);
	IloExpr expr(env);
	IloExpr expr_left(env);
	IloExpr num_back_train_position(env);
	for(i=0;i<station;i++)
	{
		model.add(backup_train_position[i]==solution_z[i]);
	}
	
for(i=0;i<station;i++)
	{
		IloExpr back_flow(env);
		for(j=0;j<T_stamp;j++)
		{
			back_flow += backuptrain_spatial_dummy_origin[i*T_stamp+j];
		}
		model.add(back_flow==backup_train_position[i]);
	}
	for(i=0;i<station;i++)
	{
		num_back_train_position += backup_train_position[i];
	}
	model.add(Arc_selection[0]==0);
	model.add(Passenger_flow_variable[0]==0);
// Flow balance constraints
	IloExpr dummy_destination(env);
	for(i=0;i<Space_Time_Node;i++)
	{
		IloExpr flow_in(env), flow_out(env);
		if(find(train_esist_position, i)==1)
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)]+1+backuptrain_spatial_dummy_origin[i];
			flow_out = Arc_selection[out_travel_arc_find(i)]+Arc_selection[out_waiting_arc_find(i)];
			model.add(flow_out==flow_in);
		}
		else if(i>=(station-1)*T_stamp)
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)];
			dummy_destination += flow_in;
			//	flow_out = Arc_selection[w][out_travel_arc_find(i)]+Arc_selection[w][out_waiting_arc_find(i)]+1;
				//model.add(flow_in==flow_out);
		}
		else
		{
			flow_in  = Arc_selection[in_travel_arc_find(i)]+Arc_selection[in_waiting_arc_find(i)]+backuptrain_spatial_dummy_origin[i];
			flow_out = Arc_selection[out_travel_arc_find(i)]+Arc_selection[out_waiting_arc_find(i)];
			model.add(flow_in==flow_out);
		}	
	}
	model.add(dummy_destination==train_exist+num_back_train_position);
	// Headway constraints
	for(i=0;i<station;i++)
	{
		for(j=h_min-1;j<T_stamp;j++)
		{
			IloExpr flow_in(env);
			for(int t=0;t<h_min;t++)
			{
				flow_in += Arc_selection[in_travel_arc_find(i*T_stamp+j-h_min-1+t)];
			}
			model.add(flow_in<=1);
		}
	}
	for(i=0;i<space_time_travel_arc+1;i++)
	{
		model.add(Passenger_flow_variable[index_travel_time_graph[i][0]] <= Arc_selection[index_travel_time_graph[i][0]]*train_capacity);
	}
		for(i=0;i<station;i++)
		{
			for(j=0;j<T_stamp;j++)
			{
				IloExpr flow_in(env), flow_out(env);
				flow_in  = Passenger_flow_variable[in_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[in_waiting_arc_find(i*T_stamp+j)];
				flow_out = Passenger_flow_variable[out_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[out_waiting_arc_find(i*T_stamp+j)];
				if(j==0)
				{
					model.add(flow_out==origin_total[i][w]);
					model.add(passenger_alighting[i][j]==0);
				}
				else if(j!=T_stamp-1)
				{
					model.add(passenger_alighting[i][j] == passenger_time_board[i][j][w] + flow_in-flow_out);
					model.add(passenger_alighting[i][j] <= Arc_selection[in_travel_arc_find(i*T_stamp+j)]*big_M);
				}
			}
		}
		for(i=0; i<station; i++)
		{
			IloExpr total_passenger_destination(env);
			for(j=1; j<T_stamp-1; j++)
			{
				IloExpr total_passenger_destination_current(env);
				for(int t=0; t<j+1; t++)
				{
					total_passenger_destination_current += passenger_alighting[i][t]; 
				}
				model.add(total_passenger_destination_current<= passenger_time_alight[i][j][w]);
				total_passenger_destination += passenger_alighting[i][j];
			}
			expr += (passenger_time_alight[i][T_stamp-2][w] - total_passenger_destination)*T_stamp*penlty;
		}
	// Objective Function
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			expr += j*passenger_alighting[i][j];
		}
	}
	//expr += expr_left*w_l;
	/*
	for(i=0;i<station;i++)
	{
		expr += backup_train_position[i]*investment_cost;
	}*/
	// Objective Function
	IloObjective obj = IloMinimize (env, expr);
	model.add(obj);
	// Objective function finished
	IloCplex newcc(model);
	// newcc.setParam(IloCplex::TiLim, 1000.0);
	newcc.setParam(IloCplex::TiLim, 3000.0);
	newcc.setOut(env.getNullStream());
	//newcc.setParam(IloCplex::);
	newcc.solve();
	cout<<"Solution status = "<<newcc.getCplexStatus()<<endl;
	cout<<"Objective value = "<<newcc.getObjValue()<<endl;
	test_objective = newcc.getObjValue();
	cout<<"The gap is "<<(newcc.getObjValue()-newcc.getBestObjValue())/(newcc.getObjValue()*100+1)<<endl;
	float judge;
	int xq=0;
	for(i=0;i<Space_Time_Arc_Num+1;i++)
	{
		judge = newcc.getValue(Arc_selection[i]);
		if(judge - (int)judge != 0)
		{
			cout<<judge<<" "<<endl;
		}
	}
	cout<<"This is an integer solution? : = "<<xq<<endl;
	for(i=0;i<station;i++)
	{
		if(i==0)
		{
			used_depot_trains=newcc.getValue(backup_train_position[i]);
		}
		else
		{
		used_back_up_trains += newcc.getValue(backup_train_position[i]);
		}
	}
	// Result display and storage
	model_objective=newcc.getObjValue();
	objective_original_subproblems[w]=model_objective;
	float ob_1=0, alighting[station]={0};
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			alighting[i] += newcc.getValue(passenger_alighting[i][j]); 
		}
		ob_1 += (passenger_time_alight[i][T_stamp-2][w]-alighting[i]);
	}
	cout<<"Objective composition: "<<endl;
	cout<<"Total num passengers:  "<<total_passenger_demand[w]<<endl;
	cout<<"OB1: Detained passenger: "<<ob_1<<endl;
	cout<<"OB2: Travelling time: "<<model_objective-ob_1*w_l<<endl;
    //Data storage
	/*
	errno_t storage_space_time_flows;
	storage_space_time_flows = fopen_s(&space_time_network_flows, "D:/Backup_train/space_time_network_flows.txt","w");
	int c_a=0;
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_a=newcc.getValue(Arc_selection[k]);
		fprintf(space_time_network_flows, "%d ", c_a);
	}
	fclose(space_time_network_flows);
	errno_t p_flow;
	float c_b=0;
	p_flow = fopen_s(&passenger_flows, "D:/Backup_train/passenger_flows.txt","w");
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_b=newcc.getValue(Passenger_flow_variable[k]);
		fprintf(passenger_flows, "%f ", c_b);
	}
	fclose(passenger_flows);
	
	errno_t storage_alighting_passengers;
	storage_alighting_passengers = fopen_s(&alighting_passengers, "D:/Backup_train/space_time_network_alighting.txt","w");
    float a_flow=0;
	for(i=0; i<station; i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			a_flow=newcc.getValue(passenger_alighting[i][j]);
			fprintf(alighting_passengers, "%f ", a_flow);
		}
		fprintf(alighting_passengers, "\n");
	}
	fclose(alighting_passengers);*/
	newcc.end();
	env.end();
	
}  

void time_dependent_solver_space_time_backup_integer_multi_scenario_new()
{
	int i,j,k,q;
	int in_1, in_2, out_1, out_3; 
	IloEnv env;
	IloNumVarArray backup_train_position(env, station);
	for(i=0;i<station;i++)
	{
		backup_train_position[i]=IloNumVar(env,0, 1, ILOINT);
	}
	NumVarMatrix backuptrain_spatial_dummy_origin(env, num_scenario);
	for(k=0;k<num_scenario;k++)
	{	
		backuptrain_spatial_dummy_origin[k]=IloNumVarArray(env, Space_Time_Node);
		for(i=0;i<Space_Time_Node;i++)
		{
			backuptrain_spatial_dummy_origin[k][i]=IloNumVar(env, 0, 1, ILOINT);
		}
	}
	NumVarMatrix Arc_selection(env, num_scenario);
	NumVarMatrix Passenger_flow_variable(env, num_scenario);
	for(k=0;k<num_scenario;k++)
	{
		Arc_selection[k] = IloNumVarArray(env, Space_Time_Arc_Num+1);
		Passenger_flow_variable[k] = IloNumVarArray(env, Space_Time_Arc_Num+1);
		for(i=0;i<Space_Time_Arc_Num+1;i++)
		{
			Arc_selection[k][i]=IloNumVar(env, 0, 1, ILOINT);
			Passenger_flow_variable[k][i]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
		}
	}
	NumVar3Matrix passenger_boarding(env, num_scenario);
	NumVar3Matrix passenger_alighting(env, num_scenario);
	for(k=0;k<num_scenario;k++)
	{
		passenger_boarding[k]= NumVarMatrix(env, station);
		passenger_alighting[k]= NumVarMatrix(env, station);
		for(i=0;i<station;i++)
		{
			passenger_boarding[k][i]=IloNumVarArray(env, T_stamp);
			passenger_alighting[k][i]=IloNumVarArray(env, T_stamp);
			for(j=0;j<T_stamp;j++)
			{
				passenger_boarding[k][i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
				passenger_alighting[k][i][j]=IloNumVar(env, 0, IloInfinity, ILOFLOAT);
			}
		}
	}
	IloModel model(env);
	IloExpr expr(env);
	IloExpr expr_left(env);
	IloExpr num_back_train_position(env);
/*	for(int i=0;i<station;i++)
	{
		model.add(backup_train_position[i]==1);
	}*/
	for(k=0;k<num_scenario;k++)
	{
		for(i=0;i<station;i++)
		{
			IloExpr back_flow(env);
			for(j=0;j<T_stamp;j++)
			{
				back_flow += backuptrain_spatial_dummy_origin[k][i*T_stamp+j];
			}
			model.add(back_flow==backup_train_position[i]);
		}
		for(i=0;i<station;i++)
		{
			num_back_train_position += backup_train_position[i];
		}
		model.add(Arc_selection[k][0]==0);
		model.add(Passenger_flow_variable[k][0]==0);
// Flow balance constraints
		IloExpr dummy_destination(env);
		for(i=0;i<Space_Time_Node;i++)
		{
			IloExpr flow_in(env), flow_out(env);
			if(find(train_esist_position, i)==1)
		//	if(i==0||i==T_stamp*2||i==T_stamp*5||i==T_stamp*6)
			{
				flow_in  = Arc_selection[k][in_travel_arc_find(i)]+Arc_selection[k][in_waiting_arc_find(i)]+1+backuptrain_spatial_dummy_origin[k][i];
				flow_out = Arc_selection[k][out_travel_arc_find(i)]+Arc_selection[k][out_waiting_arc_find(i)];
				model.add(flow_out==flow_in);
			}
		//	else if(i==Space_Time_Node-40||i==Space_Time_Node-30||i==Space_Time_Node-20||i==Space_Time_Node-1)
			else if(i>=(station-1)*T_stamp)
			{
				flow_in  = Arc_selection[k][in_travel_arc_find(i)]+Arc_selection[k][in_waiting_arc_find(i)];
				dummy_destination += flow_in;
			//	flow_out = Arc_selection[w][out_travel_arc_find(i)]+Arc_selection[w][out_waiting_arc_find(i)]+1;
				//model.add(flow_in==flow_out);
			}
			else
			{
				flow_in  = Arc_selection[k][in_travel_arc_find(i)]+Arc_selection[k][in_waiting_arc_find(i)]+backuptrain_spatial_dummy_origin[k][i];
				flow_out = Arc_selection[k][out_travel_arc_find(i)]+Arc_selection[k][out_waiting_arc_find(i)];
				model.add(flow_in==flow_out);
			}	
		}
		model.add(dummy_destination==train_exist + num_back_train_position);
	// Headway constraints
		for(i=0;i<station;i++)
		{
			for(j=h_min-1;j<T_stamp;j++)
			{
				IloExpr flow_in(env);
				for(int t=0;t<h_min;t++)
				{
					flow_in += Arc_selection[k][in_travel_arc_find(i*T_stamp+j-h_min-1+t)];
				}
				model.add(flow_in<=1);
			}
		}
	// Passenger flow constraints
	// Capacity constraints
		for(i=0;i<space_time_travel_arc+1;i++)
		{
			model.add(Passenger_flow_variable[k][index_travel_time_graph[i][0]] <= Arc_selection[k][index_travel_time_graph[i][0]]*train_capacity);
		}
		for(i=0;i<station;i++)
		{
			for(j=0;j<T_stamp;j++)
			{
				IloExpr flow_in(env), flow_out(env);
				flow_in  = Passenger_flow_variable[k][in_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[k][in_waiting_arc_find(i*T_stamp+j)];
				flow_out = Passenger_flow_variable[k][out_travel_arc_find(i*T_stamp+j)]+Passenger_flow_variable[k][out_waiting_arc_find(i*T_stamp+j)];
				if(j==0)
				{
					model.add(flow_out==origin_total[i][k]);
					model.add(passenger_alighting[k][i][j]==0);
				}
				else if(j!=T_stamp-1)
				{
					model.add(passenger_alighting[k][i][j] == passenger_time_board[i][j][k] + flow_in-flow_out);
					model.add(passenger_alighting[k][i][j] <= Arc_selection[k][in_travel_arc_find(i*T_stamp+j)]*big_M);
				}
			}
		}
		for(i=0; i<station; i++)
		{
			IloExpr total_passenger_destination(env);
			for(j=1; j<T_stamp-1; j++)
			{
				IloExpr total_passenger_destination_current(env);
				for(int t=0; t<j+1; t++)
				{
					total_passenger_destination_current += passenger_alighting[k][i][t]; 
				}
				model.add(total_passenger_destination_current<= passenger_time_alight[i][j][k]);
				total_passenger_destination += passenger_alighting[k][i][j];
			}
			expr += (passenger_time_alight[i][T_stamp-2][k] - total_passenger_destination)*T_stamp*p[k]*penlty;
		//T_stamp
		}
	}
	// Objective Function
	for(k=0;k<num_scenario;k++)
	{
		for(i=0;i<station;i++)
		{
			for(j=0;j<T_stamp-1;j++)
			{
				expr += j*passenger_alighting[k][i][j]*p[k];
			}
		}
	}
	//expr += expr_left*w_l;
	for(i=0;i<station;i++)
	{
		expr += backup_train_position[i]*investment_cost;
	}
	IloObjective obj = IloMinimize (env, expr);
	model.add(obj);
	// Objective function finished
	IloCplex newcc(model);
	newcc.setParam(IloCplex::TiLim, 19200.0);
	// newcc.setParam(IloCplex::AggCutLim, -1);
	 newcc.setParam(IloCplex::FlowCovers, -1);
	 newcc.setParam(IloCplex::MIRCuts, -1);
	newcc.solve();
	cout<<"Solution status = "<<newcc.getCplexStatus()<<endl;
	cout<<"Objective value = "<<newcc.getObjValue()<<endl;
	cout<<"The gap is "<<(newcc.getObjValue()-newcc.getBestObjValue())/(newcc.getObjValue()*100+1)<<endl;
	cout<<"Optimal solution display: "<<endl;
	for(i=0;i<station;i++)
	{
		cout<<newcc.getValue(backup_train_position[i])<<" "<<endl;
	}
	for(i=0;i<station;i++)
	{
		if(i==0)
		{
			used_depot_trains=newcc.getValue(backup_train_position[i]);
		}
		else
		{
		used_back_up_trains += newcc.getValue(backup_train_position[i]);
		}
	}
	// Result display and storage
	model_objective=newcc.getObjValue();
	float ob_1=0, alighting[station]={0};
	for(i=0;i<station;i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			alighting[i] += newcc.getValue(passenger_alighting[0][i][j]); 
		}
		ob_1 += (destination_total[i][0]-alighting[i]);
	}
	cout<<"Objective composition: "<<endl;
	cout<<"Total num passengers:  "<<total_passenger_demand[0]<<endl;
	cout<<"OB1: Detained passenger: "<<ob_1<<endl;
	cout<<"OB2: Travelling time: "<<model_objective-ob_1*w_l<<endl;
	

    //Data storage
/*	errno_t storage_space_time_flows;
	storage_space_time_flows = fopen_s(&space_time_network_flows, "D:/Backup_train/space_time_network_flows.txt","w");
	int c_a=0;
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_a=newcc.getValue(Arc_selection[k]);
		fprintf(space_time_network_flows, "%d ", c_a);
	}
	fclose(space_time_network_flows);
	errno_t p_flow;
	float c_b=0;
	p_flow = fopen_s(&passenger_flows, "D:/Backup_train/passenger_flows.txt","w");
	for(int k=0;k<Space_Time_Arc_Num+1;k++)
	{
		c_b=newcc.getValue(Passenger_flow_variable[k]);
		fprintf(passenger_flows, "%f ", c_b);
	}
	fclose(passenger_flows);
	
	errno_t storage_alighting_passengers;
	storage_alighting_passengers = fopen_s(&alighting_passengers, "D:/Backup_train/space_time_network_alighting.txt","w");
    float a_flow=0;
	for(i=0; i<station; i++)
	{
		for(j=0;j<T_stamp-1;j++)
		{
			a_flow=newcc.getValue(passenger_alighting[i][j]);
			fprintf(alighting_passengers, "%f ", a_flow);
		}
		fprintf(alighting_passengers, "\n");
	}
	fclose(alighting_passengers);*/
}  

void master_problem(int ite)
{
	int i, j, w, k;
	IloEnv env_master;
	IloNumVarArray back_position(env_master, station);
//	IloNumVarArray theta(env_master, num_scenario);
	for(i=0;i<station;i++)
	{
		back_position[i]=IloNumVar(env_master, 0, 1, ILOINT);
	}
/*	for(w=0;w<num_scenario; w++)
	{
		theta[w]=IloNumVar(env_master, 0, IloInfinity, ILOFLOAT);
	}*/
	IloNumVar theta(env_master, -IloInfinity, IloInfinity, ILOFLOAT);
	IloModel model(env_master);
	
	// Constraints:	
/*	for(w=0;w<num_scenario; w++)
	{
		model.add(theta[w]>=objective_original_subproblems[w]);
	}*/
	//for(k=0;k<ite;k++)
	//{
	
	/*for(i=0;i<station;i++)
	{
		sum_x += solution_z[i]*back_position[i]-(1-solution_z[i])*back_position[i]-solution_z[i];
	}*/
	model.add(back_position[6-1]==0);
	model.add(back_position[18-1]==0);
	model.add(back_position[21-1]==0);
	int a_string[station] = {0};
	for(i=0;i<station;i++)
	{
		a_string[i]=(i+1)*100;
	}
	for(k=0;k<ite+1;k++)
	{
		IloExpr sum_x(env_master);
		for(i=0;i<station;i++)
		{
			sum_x += a_string[i]*solution_z_matrix[k][i]*back_position[i]-a_string[i]*(1-solution_z_matrix[k][i])*back_position[i]-a_string[i]*solution_z_matrix[k][i];
		}
		//model.add(theta>=(sum_x+1)*(Qx_matrix[k]-lower_bound_second_stage)+lower_bound_second_stage);
		model.add(theta>= sum_x*(Qx_matrix[k] - lower_bound_second_stage) + Qx_matrix[k]);
		// model.add(theta>= lower_bound_second_stage);
	}
/*	IloExpr total_num(env_master);
	for(i=0;i<station;i++)
	{
		total_num += back_position[i];
	}*/
	/*if(ite%2==0)
	{
		model.add(total_num >=5);
	}
	else
	{
		model.add(total_num<5);
	}*/
	// cutting_list[][]=
	//model.add(theta>=(sum_x+1)*(Qx-lower_bound_second_stage)+lower_bound_second_stage);
	//}
    // Objective value
	IloExpr expr(env_master);
	for(i=0; i<station; i++)
	{
		expr += back_position[i]*investment_cost;
	}
	expr += theta;
	IloObjective obj = IloMinimize (env_master, expr);
	model.add(obj);
	// Objective function finished
	IloCplex newcc(model);
	newcc.solve();
	lower_bound_current[ite]=newcc.getObjValue();
	for(i=0;i<station;i++)
	{
		solution_z[i]=newcc.getValue(back_position[i]);
		solution_z_matrix[ite+1][i]=newcc.getValue(back_position[i]);
	}
	cout<<"Solution status = "<<newcc.getCplexStatus()<<endl;
	cout<<"Objective value = "<<newcc.getObjValue()<<endl;
}
// BENDERS DECOMPOSITION ALGORITHM
void benders_decomposition()
{
	// INITIALIZE
	float display_1[num_scenario], display_2[num_scenario];
	int i, j, k, w;
	for(i=0;i<station;i++)
	{
		solution_z[i]=1;
	}
	// Compute the lower bound: L - Q(x)
	int s_array[num_scenario];
	for(int i=0;i<num_scenario;i++)
	{
		s_array[i]=i;
	}
	int s=0;

	parallel_for_each(begin(s_array), end(s_array), [&](int s)
	{
		display_1[s]=integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_1(s);
		lower_bound_second_stage += p[s]*objective_original_subproblems[s];
	});
	
	/*for(w=0; w<num_scenario;w++)
	{
		display_1[w]=integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_1(w);
		lower_bound_second_stage += p[w]*objective_original_subproblems[w];
	}*/
	// Initilized solution
	for(i=0;i<station;i++)
	{
		solution_z[i]=1;
		solution_z[6-1]=0;
		solution_z[18-1]=0;
		solution_z[21-1]=0;
	//	solution_z[i]=0;
		solution_z[i]=0;
		solution_z_matrix[0][i]=solution_z[i];
	}	
	//solution_z[0]=1;solution_z[1]=1; solution_z[3]=1; solution_z[2]=1;solution_z[4]=1;solution_z[5]=1;solution_z[6]=1;solution_z[7]=1;
	//cout<<"The lower bound is "<<lower_bound_second_stage<<endl;
	
	
	for(k=0;k<max_iteration;k++)
	{
		cout<<"This is the "<<k<<"th iteration"<<endl;
		// Subproblems
		Qx=0;		
		for(i=0;i<station;i++)
		{
			upper_bound_current[k] += solution_z[i]*investment_cost;
		}
		s=0;
		
	parallel_for_each(begin(s_array), end(s_array), [&](int s)
	{
			display_2[s] = integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_1(s);
			Qx += objective_original_subproblems[s]*p[s];
			upper_bound_current[k] += p[s]*objective_original_subproblems[s];
	});
	/*	for(s=0;s<num_scenario;s++)
		{
			display_2[w] = integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_1(s);
			Qx += objective_original_subproblems[s]*p[s];
			// solver_space_time_backup_linear_single_scenario_revised(w);
			upper_bound_current[k] += p[s]*objective_original_subproblems[s];
		}*/
		Qx_matrix[k]=Qx;
		cout<<"Test: "<<upper_bound_current[k]<<endl;
		// Master problem
		 master_problem(k);
		 cout<<"%%%%%%%%%%%%%%%%%%%%%%"<<lower_bound_second_stage<<endl;
		// Update upper bound
	}
	/*cout<<"Test the benders decomposition with respect to parallel"<<endl;
	for(s=0;s<num_scenario;s++)
	{
		cout<<display_1[s]<<" ";
	}
	cout<<endl;
	for(s=0;s<num_scenario;s++)
	{
		cout<<display_2[s]<<" ";
	}*/
}


void L_METHOD_benders_decomposition()
{
	// INITIALIZE
	int i, j, k, w;
	for(i=0;i<station;i++)
	{
		solution_z[i]=1;
	}
	// Compute the lower bound: L - Q(x)
	int s_array[num_scenario];
	for(int i=0;i<num_scenario;i++)
	{
		s_array[i]=i;
	}
	int s=0;
	for(w=0; w<num_scenario;w++)
	{
		integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_2(w);
		lower_bound_second_stage += p[w]*objective_original_subproblems[w];
	}
	// Initilized solution
	for(i=0;i<station;i++)
	{
		solution_z[i]=1;
		solution_z[23]=0;
		solution_z[22]=0;
		solution_z[21]=0;
		solution_z_matrix[0][i]=solution_z[i];
	}	
	//solution_z[0]=1;solution_z[1]=1; solution_z[3]=1; solution_z[2]=1;solution_z[4]=1;solution_z[5]=1;solution_z[6]=1;solution_z[7]=1;
	//cout<<"The lower bound is "<<lower_bound_second_stage<<endl;
	
	for(k=0;k<max_iteration;k++)
	{
		cout<<"This is the "<<k<<"th iteration"<<endl;
		// Subproblems
		Qx=0;		
		for(i=0;i<station;i++)
		{
			upper_bound_current[k] += solution_z[i]*investment_cost;
		}
		s=0;
		for(s=0;s<num_scenario;s++)
		{
			integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_2(s);
			Qx += objective_original_subproblems[s]*p[s];
			// solver_space_time_backup_linear_single_scenario_revised(w);
			upper_bound_current[k] += p[s]*objective_original_subproblems[s];
		}
		Qx_matrix[k]=Qx;
		cout<<"Test: "<<upper_bound_current[k]<<endl;
		// Master problem
		 master_problem(k);
		 cout<<"%%%%%%%%%%%%%%%%%%%%%%"<<lower_bound_second_stage<<endl;
		// Update upper bound
	}
}

int _tmain()
{
	cout<<"Instance yyy: "<<num_scenario<<endl;
    data_loading();
	 clock_t start,finish;
	double totaltime;
	start=clock();
	srand(unsigned (time(NULL)));
	// Initialize the parameters of passenger demands
	stochastic_parameter_setting();
	cout<<"%%&& "<<origin_total[0][0]<<" "<<origin_total[1][0]<<" "<<origin_total[2][0]<<" "<<origin_total[3][0]<<" "<<origin_total[4][0]<<endl;
	cout<<"DES ";
	for(int i=0;i<station;i++)
	{
		cout<<destination_total[i][0]<<" ";
	}
    // Construct the spatial-time network	
	Space_Time_Network(arc_network);
	int origin_station, origin_time, destination_station, destination_time;
	int test_value=0;
    for (int i = 0; i < Space_Time_Node; i++)
    {
        for (int j = 0; j < Space_Time_Node; j++)
        {
            if (arc_network[i][j]!=0)
            {
				origin_station=i/T_stamp;
				origin_time=i%T_stamp;
				destination_station=j/T_stamp;
				destination_time=j%T_stamp;
				//cout << vextex[j] << ",";
				Space_Time_Arc_Num +=1;	
				if(origin_station!=destination_station)
				{
					space_time_travel_arc +=1;
					index_travel_time_graph[space_time_travel_arc][0]=Space_Time_Arc_Num;
				}
				//cout << "[("<<origin_station<<","<<origin_time<<"),("<<destination_station<<","<<destination_time<<")]"<<" ";
            }
        }
    }
	 benders_decomposition();
	// L_METHOD_benders_decomposition();
	/*
		for(int i=0;i<station;i++)
	{
		solution_z[i]=1;
	}
		float a_1, a_2;
		integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_2(0);
		a_1 = objective_original_subproblems[0];
		integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_1(0);
		a_2 = objective_original_subproblems[0];
		cout<<"If they are different? = "<<a_1-a_2<<endl;*/
	// benders_decomposition();
	// solver_space_time_backup_integer_multi_scenario_new();
	// solver_space_time_backup_integer_single_scenario_revised(1);
	  //time_dependent_solver_space_time_backup_linear_single_scenario_revised(0);
	// float ob_1, ob_2;
	// integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_1(0);
	 //ob_1 = test_objective;
	 //integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_1(1);
	 // ob_2 = test_objective;
	 // cout<<"%%%%%%%%%%% Value "<<ob_1-ob_2<<endl;
//     time_dependent_solver_space_time_backup_integer_multi_scenario_new();
	 //integer_time_dependent_solver_space_time_backup_linear_single_scenario_revised_2(0);
	cout<<endl;
	cout<<"%%%% Debug Here %%%%%%%"<<endl;
	cout<<"Total number of passengers at scenario 0 is "<<total_passenger_demand[0]<<endl;;
	cout << "The number of arcs in the space-time network "<<Space_Time_Arc_Num<<endl;
	cout << "The number of travel arcs in the space-time network "<<space_time_travel_arc<<endl;
	cout << "Travel arc num: "<<in_travel_arc_find(1)<<";"<<"Waiting arc num: "<<in_waiting_arc_find(2)<<endl;
	cout<<"Trave arc out: "<<out_travel_arc_find(1)<<";"<<"Wait arc out: "<<out_waiting_arc_find(1)<<endl;
	//cout<<arc_network[0][T_stamp+t_running/t_interval]<<endl;
	//solver_backup();
	finish=clock();
	totaltime=(double)(finish-start)/CLOCKS_PER_SEC;
	cout<<"Total Computing Time "<<totaltime<<"Seconds"<<endl;
	
	// Store the space-time-network

	errno_t space_time_network_store;
	space_time_network_store = fopen_s(&space_time_network_flows, "D:/Backup_train/space_time_network_storage.txt","w");
	for(int i=0;i<Space_Time_Node;i++)
	{
		for(int j=0;j<Space_Time_Node;j++)
		{
			fprintf(space_time_network_flows, "%d ", arc_network[i][j]);
		}
		fprintf(space_time_network_flows, "\n");
	}
	fclose(space_time_network_flows);

	// Test part 
	cout<<"%% Number of used backup trains:     "<<used_back_up_trains<<endl;
	cout<<"%% Number of trains from depots:     "<<used_depot_trains<<endl;
	cout<<"%% Passenger total travel time:     "<<(model_objective-used_back_up_trains*investment_cost-used_depot_trains*investment_cost_depot)<<endl;
	cout<<"%% Passenger average travel time:   "<<(model_objective-used_back_up_trains*investment_cost-used_depot_trains*investment_cost_depot)/total_passenger_demand[0]<<endl;
	cout<<" Lower and upper bound "<<objective_relaxed_subproblems[0]<<" "<<objective_original_subproblems[0]<<endl;
	cout<<" The value of Qx is "<<Qx<<endl;
	cout<<endl;
	cout<<"Upper bound: ";
	for(int i=0;i<max_iteration;i++)
	{
		cout<<upper_bound_current[i]<<" ";
	}
	cout<<endl;
	cout<<"Lower bound: ";
	for(int i=0;i<max_iteration;i++)
	{
		cout<<lower_bound_current[i]<<" ";
	}
	cout<<"The final optimal solution is "<<endl;
	for(int i=0;i<station;i++)
	{
		cout<<solution_z[i]<<" ";
	}
	errno_t save_results;
	save_results = fopen_s(&bound_save, "D:/Backup_train/bound_save.txt","w");
	for(int i=0;i<max_iteration;i++)
	{
		fprintf(bound_save, "%f ", upper_bound_current[i]);
	}
	fprintf(bound_save, "\n");
	for(int i=0;i<max_iteration;i++)
	{
		fprintf(bound_save, "%f ", lower_bound_current[i]);
	}
	fclose(bound_save);
	cout<<"Test "<<lower_bound_second_stage<<endl;
	for(int k=0;k<max_iteration;k++)
	{
		cout<<"Value of Qx in iteration "<<k<< " = "<< Qx_matrix[k]<<endl;
	}
	cout<<"Optimal Solution In each Iteration"<<endl;
	for(int k=0;k<max_iteration+1;k++)
	{
		for(int i=0;i<station;i++)
		{
			cout<<solution_z_matrix[k][i]<<" ";
		}
		cout<<endl;
	}
	exit(0);	
}

