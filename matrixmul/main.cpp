#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <iostream>
#include "mmul.h"
#include "omp-tools.h"

//#include "compute.h"

using namespace std;

void callback_parallel_begin(
	ompt_data_t* encountering_task_data,         /* data of encountering task           */
	const ompt_frame_t *encountering_task_frame,  /* frame data of encountering task     */
	ompt_data_t *parallel_data,                  /* data of parallel region             */
	unsigned int requested_parallelism,          /* requested number of threads in team */
	int flag,                                    /* flag for invocation attribute       */
	const void *codeptr_ra                       /* return address of runtime call      */
) {
	printf("[INFO] parallel begin (%u threads requested)\n", requested_parallelism);
	//system("pause");
}

void callback_parallel_end(
	ompt_data_t *parallel_data,           /* data of parallel region             */
	ompt_data_t *encountering_task_data,  /* data of encountering task           */
	int flag,                             /* flag for invocation attribute       */
	const void *codeptr_ra                /* return address of runtime call      */
) {
	printf("[INFO] parallel end\n");
}

void callback_thread_begin(
	ompt_thread_t thread_type,            /* type of thread                      */
	ompt_data_t *thread_data              /* data of thread                      */
) {
	printf("[INFO] %s (%d) thread start (%llu)\n", thread_type == ompt_thread_worker ? "Worker" : "Other", thread_type, thread_data->value);
}

void callback_thread_end(
	ompt_data_t *thread_data              /* data of thread                      */
) {
	printf("[INFO] thread end (%llu)\n", thread_data->value);
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

int main(int argc, char* argv[])
{
	int n;
	int num_threads;
	if (argc != 3) {
		cout << "Incorrect input. Try again.\n" <<
			"    The input data should contain:\n 1)number of threads in integer;\n 2)dimention of matrix in integer shared by space;\n";
		exit(1);
	}
	else {
		n = atoi(argv[2]);
		num_threads = atoi(argv[1]);
	}

	if (num_threads < 1) {
		cout << "\n" << "Please, enter the correct number of threads in integer\n";
		exit(1);
	}
	omp_set_num_threads(num_threads);

	mmul(make_matrix_of_1(n), make_matrix_of_2(n), make_matrix(n), n);
	//system("pause");
	return 0;
}//*/