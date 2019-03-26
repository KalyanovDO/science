#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <iostream>
#include "mmul.h"
#include "jakobi.h"
#include "monte-karlo.h"
#include "omp-tools.h"
#include <map>
#include <vector>
#include <algorithm>


//#include "compute.h"

using namespace std;

vector<pair<int, double>> time_points; /*container of points of time at main events*/

void time_laps(void) {
	vector <pair <int, double>> ::iterator it = time_points.begin();

	for (int i = 0; it != time_points.end(); it++, i++) {  // выводим их
		cout << i << ") Key " << it->first << ", value " << it->second << endl;
	}
}

typedef enum event_t {
	parallel_event = 1,
	thread_event = 2
} event_t;

pair<vector <pair <int, double>> ::iterator, double> func_search_parallel_end(vector<pair<int, double>> time_points) {
	double parallel_end = 0;
	vector <pair <int, double>> :: iterator buf_iter = time_points.begin();

	for (; buf_iter < time_points.end(); buf_iter++) {
		if (buf_iter->first == ompt_callback_thread_end) {
			parallel_end = buf_iter->second;
			return make_pair(buf_iter,parallel_end);
		}
	}
	return make_pair(buf_iter, parallel_end);
}


vector<pair<event_t, double>> time_spans;  /*container of time spans depended on event*/

//функция, превращающая вектор временных точек в вектор временных отрезков
vector<pair<event_t, double>> time_spansing (vector<pair<int, double>> time_points) {

	vector <pair <int, double>>::iterator it = time_points.begin();

	it = time_points.begin();
	/*сначала находим конец потоков первый по счёту, находим все времена работы каждого из потоков
	после этого находим первую пару начало - конец параллельной зоны и вычисляем время разностным способом*/
	while (!time_points.empty()) {

		
		vector <pair <int, double>> ::iterator buf_iter = time_points.begin();

		//while (it->second != ompt_callback_thread_end || !time_points.empty()) {

		pair<vector <pair <int, double>> ::iterator, double> fff = func_search_parallel_end(time_points);
		double parallel_end = fff.second;
		vector <pair <int, double>> ::iterator buf_iter1 = fff.first;
			for (buf_iter = time_points.begin(); buf_iter < time_points.end()/*buf_iter1*/; buf_iter++) {
				if (buf_iter->first == (int) ompt_callback_thread_begin) {
					time_spans.push_back(make_pair(thread_event, parallel_end - buf_iter->second));
				}
			}

			//it++;
		//}
			//all parallel events until end of threads
		//while (/*it != buf_iter1 ||*/ it != time_points.end()) {
			double buf_begin = 0;
			for (it = time_points.begin(); it < time_points.end()/*buf_iter1*/; it++) {
	
				if (it->first == (int) ompt_callback_parallel_begin) {
					buf_begin = it->second;
				}

				//it++;

				if (it->first == (int) ompt_callback_parallel_end) {
					time_spans.push_back(make_pair(parallel_event, it->second - buf_begin));
					buf_begin = 0;
				}
			}
		//}
		time_points.erase(time_points.begin(), time_points.end()/*buf_iter1*/);
	}


	return time_spans;
}

void funcd(void) {
	vector <pair <event_t, double>> fff = time_spansing(time_points);
	vector <pair <event_t, double>> ::iterator it = fff.begin();
	//cout << "А вот все отсортированно: " << endl;
	for (int i = 0; it != fff.end(); it++, i++) {  // выводим их
		cout << i << ") Key " << it->first << ", value " << it->second << endl;
	}
}


void callback_parallel_begin(
	ompt_data_t* encountering_task_data,         /* data of encountering task           */
	const ompt_frame_t *encountering_task_frame,  /* frame data of encountering task     */
	ompt_data_t *parallel_data,                  /* data of parallel region             */
	unsigned int requested_parallelism,          /* requested number of threads in team */
	int flag,                                    /* flag for invocation attribute       */
	const void *codeptr_ra                       /* return address of runtime call      */
) {
	printf("[INFO] parallel begin (%u threads requested)\n", requested_parallelism);
	time_points.push_back(make_pair(ompt_callback_parallel_begin, omp_get_wtime()));
}

void callback_parallel_end(
	ompt_data_t *parallel_data,           /* data of parallel region             */
	ompt_data_t *encountering_task_data,  /* data of encountering task           */
	int flag,                             /* flag for invocation attribute       */
	const void *codeptr_ra                /* return address of runtime call      */
) {
	time_points.push_back(make_pair(ompt_callback_parallel_end, omp_get_wtime()));
	printf("[INFO] parallel end\n");
}

void callback_thread_begin(
	ompt_thread_t thread_type,            /* type of thread                      */
	ompt_data_t *thread_data              /* data of thread                      */
) {
	printf("[INFO] %s (%d) thread start (%llu)\n", thread_type == ompt_thread_worker ? "Worker" : "Other", thread_type, thread_data->value);
	time_points.push_back(make_pair(ompt_callback_thread_begin, omp_get_wtime()));
}

void callback_thread_end(
	ompt_data_t *thread_data              /* data of thread                      */
) {
	// vec::time_points.push_back(make_pair(ompt_callback_thread_end, omp_get_wtime()));
	printf("[INFO] threads end (%llu)\n", thread_data->value);
	//time_laps();
}



int ompt_initialize(ompt_function_lookup_t lookup, ompt_data_t *tool_data) {
	printf("[INFO] ompt_initialize is called\n");
	ompt_set_callback_t set_callback = (ompt_set_callback_t)lookup("ompt_set_callback");
	set_callback(ompt_callback_parallel_begin, (ompt_callback_t)callback_parallel_begin);
	set_callback(ompt_callback_parallel_end, (ompt_callback_t)callback_parallel_end);
	set_callback(ompt_callback_thread_begin, (ompt_callback_t)callback_thread_begin);
	set_callback(ompt_callback_thread_end, (ompt_callback_t)callback_thread_end);
	return 1;
}

void ompt_finalize(ompt_data_t *tool_data) {
	printf("[INFO] ompt_finalize is called\n");
	ompt_start_tool_result_t* result = (ompt_start_tool_result_t*)tool_data->ptr;
	delete[] result;
}

ompt_start_tool_result_t* ompt_start_tool(unsigned int omp_version, const char *runtime_version) {
	printf("[INFO] ompt_start_tool is called with omp_versoin = %u and runtime_version = %s\n", omp_version, runtime_version);
	ompt_start_tool_result_t* result = new ompt_start_tool_result_t;
	result->initialize = ompt_initialize;
	result->finalize = ompt_finalize;
	result->tool_data.ptr = (void*)result;
	return result;
}


int main(int argc, char* argv[]) {
	//setlocale(LC_ALL, "Russian");
	main_func(argc, argv);
	time_points.push_back(make_pair(2, omp_get_wtime()));
	time_laps();

	funcd();
	return 0;
}