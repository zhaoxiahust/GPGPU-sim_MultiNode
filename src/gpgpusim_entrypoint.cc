// Copyright (c) 2009-2011, Tor M. Aamodt, Wilson W.L. Fung, Ivan Sham,
// Andrew Turner, Ali Bakhoda, The University of British Columbia
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// Neither the name of The University of British Columbia nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "gpgpusim_entrypoint.h"
#include <stdio.h>
#include <sys/wait.h>
#include "option_parser.h"
#include "cuda-sim/cuda-sim.h"
#include "cuda-sim/ptx_ir.h"
#include "cuda-sim/ptx_parser.h"
#include "gpgpu-sim/gpu-sim.h"
#include "gpgpu-sim/icnt_wrapper.h"
#include "stream_manager.h"

#include <pthread.h>
#include <semaphore.h>

#define MAX(a,b) (((a)>(b))?(a):(b))

static int sg_argc = 3;
static const char *sg_argv[] = {"", "-config","gpgpusim.config"};

struct gpgpu_ptx_sim_arg *grid_params;

sem_t g_sim_signal_start;
sem_t g_sim_signal_finish;
sem_t g_sim_signal_exit;
time_t g_simulation_starttime;
pthread_t g_simulation_thread;

gpgpu_sim_config g_the_gpu_config;
gpgpu_sim *g_the_gpu;
stream_manager *g_stream_manager;

static void print_simulation_time();

void *gpgpu_sim_thread_sequential(void*)
{
   // at most one kernel running at a time
   bool done;
   do {
      sem_wait(&g_sim_signal_start);
      done = true;
      if( g_the_gpu->get_more_cta_left() ) {
          done = false;
          g_the_gpu->init();
          while( g_the_gpu->active() ) {
              g_the_gpu->cycle();
              g_the_gpu->deadlock_check();
          }
          g_the_gpu->print_stats();
          g_the_gpu->update_stats();
          print_simulation_time();
      }
      sem_post(&g_sim_signal_finish);
   } while(!done);
   sem_post(&g_sim_signal_exit);
   return NULL;
}

pthread_mutex_t g_sim_lock = PTHREAD_MUTEX_INITIALIZER;
bool g_sim_active = false;
bool g_sim_done = true;
extern int Process_id;						
extern int Process_waitpid[10];
extern int Process_count;
int Function_over_thread;
extern pthread_t thread_id_producer;
void *gpgpu_sim_thread_concurrent(void*)
{
    // concurrent kernel execution simulation thread


    do {
       if(g_debug_execution >= 3) {
          printf("GPGPU-Sim: *** simulation thread starting and spinning waiting for work ***\n");
          fflush(stdout);
       }
        while( g_stream_manager->empty() && !g_sim_done )
        {
			
		//	printf("KAIN:thread_create trap in while\n");
			;
		}
            //;
        if(g_debug_execution >= 3) {
           printf("GPGPU-Sim: ** START simulation thread (detected work) **\n");
           g_stream_manager->print(stdout);
           fflush(stdout);
        }
        pthread_mutex_lock(&g_sim_lock);
        g_sim_active = true;
        pthread_mutex_unlock(&g_sim_lock);
        bool active = false;
        bool sim_cycles = false;
        g_the_gpu->init();
        do {
            // check if a kernel has completed
            unsigned grid_uid = g_the_gpu->finished_kernel();
            if( grid_uid ){
           // 	g_stream_manager->register_finished_kernel(grid_uid);



extern unsigned long long block_cost[299999];
extern int End_Block_Process[5]; 
extern int Begin_Block_Process[5];
for(int i = Begin_Block_Process[Process_id]; i < End_Block_Process[Process_id]; i++)
{
	printf("Process_id is %d,Block id is %d, cycle end is %lld\n",Process_id,i,block_cost[i]); 
	fflush(stdout);
}
				g_the_gpu->print_stats();

					extern pthread_t thread_id_cluster[Cluster_Thread_Num];
    				extern int KAIN_pthread_init;//ADD by KAIN,to indicate begin
					KAIN_pthread_init = 0;// It should be used pthread_join here to assue the right, I miss it

					for(int i = 0; i < Cluster_Thread_Num;i++)
						pthread_join(thread_id_cluster[i],NULL);
/*
					for(int i = 0; i < Process_count-1; i++)
					{
						printf("before wait the child process end\n");
						fflush(stdout);
						wait(NULL);
						printf("after wait the child process end\n");
						fflush(stdout);
					}
*/
					while(Function_over_thread != Thread_Num) 
					{    
						printf("sleep for the thread over\n");
						sleep(1);
					}    

            	g_stream_manager->register_finished_kernel(grid_uid);
            }




			// launch operation on device if one is pending and can be run
			stream_operation op = g_stream_manager->front();
			op.do_operation(g_the_gpu);
            //printf("after do operation\n"); 
            // simulate a clock cycle on the GPU 
            if( g_the_gpu->active() ) { 
                g_the_gpu->cycle();
                sim_cycles = true;
                g_the_gpu->deadlock_check();
            }
            active = g_the_gpu->active() || !g_stream_manager->empty();
        } while( active );

         //printf(" after active\n"); 
        if(g_debug_execution >= 3) {
           printf("GPGPU-Sim: ** STOP simulation thread (no work) **\n");
           fflush(stdout);
        }
		if(sim_cycles) {
			g_the_gpu->update_stats();
		}
        pthread_mutex_lock(&g_sim_lock);
        g_sim_active = false;
        pthread_mutex_unlock(&g_sim_lock);
    } while( !g_sim_done );
    if(g_debug_execution >= 3) {
       printf("GPGPU-Sim: *** simulation thread exiting ***\n");
       fflush(stdout);
    }
    sem_post(&g_sim_signal_exit);
	printf("KAIN:thread_create exit\n");
    return NULL;
}

void synchronize()
{
    printf("GPGPU-Sim: synchronize waiting for inactive GPU simulation\n");
    g_stream_manager->print(stdout);
    fflush(stdout);
//    sem_wait(&g_sim_signal_finish);
    bool done = false;
    do {
        pthread_mutex_lock(&g_sim_lock);
        done = g_stream_manager->empty() && !g_sim_active;
        pthread_mutex_unlock(&g_sim_lock);
    } while (!done);
    printf("GPGPU-Sim: detected inactive GPU simulation thread\n");
    fflush(stdout);
//    sem_post(&g_sim_signal_start);
}

void exit_simulation()
{
    g_sim_done=true;
    printf("GPGPU-Sim: exit_simulation called\n");
    fflush(stdout);
    sem_wait(&g_sim_signal_exit);
    printf("GPGPU-Sim: simulation thread signaled exit\n");
    fflush(stdout);
}

extern bool g_cuda_launch_blocking;

gpgpu_sim *gpgpu_ptx_sim_init_perf()
{
   srand(1);
   print_splash();
   read_sim_environment_variables();
   read_parser_environment_variables();
   option_parser_t opp = option_parser_create();

   icnt_reg_options(opp);
   g_the_gpu_config.reg_options(opp); // register GPU microrachitecture options
   ptx_reg_options(opp);
   ptx_opcocde_latency_options(opp);
   option_parser_cmdline(opp, sg_argc, sg_argv); // parse configuration options
   fprintf(stdout, "GPGPU-Sim: Configuration options:\n\n");
   option_parser_print(opp, stdout);
   g_the_gpu_config.init();

   g_the_gpu = new gpgpu_sim(g_the_gpu_config);
   g_stream_manager = new stream_manager(g_the_gpu,g_cuda_launch_blocking);

   g_simulation_starttime = time((time_t *)NULL);

   sem_init(&g_sim_signal_start,0,0);
   sem_init(&g_sim_signal_finish,0,0);
   sem_init(&g_sim_signal_exit,0,0);

   return g_the_gpu;
}

void start_sim_thread(int api)
{
    if( g_sim_done ) {
        g_sim_done = false;
        if( api == 1 ) {
        //////////////////////////set the low priority of the Producer
        pthread_attr_t attr_P;
        struct sched_param param_P;
        pthread_attr_init(&attr_P);
        pthread_attr_setinheritsched (&attr_P,PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy (&attr_P, SCHED_FIFO);
        param_P.sched_priority = 99;
        pthread_attr_setschedparam (&attr_P, &param_P);
		pthread_attr_setdetachstate(&attr_P, PTHREAD_CREATE_JOINABLE);
        //////////////
			

    //////////////////set the CPU affinity
/*
    cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
    CPU_SET(0,&cpuset);//0-31
	
    int s = pthread_attr_setaffinity_np(&attr_P, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {   
        printf("Eror, in sim_thread_concurrent,pthread_setaffinity_np\n");
        assert(0);
    } 
*/	
	


        	printf("KAIN:pthread_create_onethread\n");
           pthread_create(&g_simulation_thread,&attr_P,gpgpu_sim_thread_concurrent,NULL);
           //pthread_create(&g_simulation_thread,NULL,gpgpu_sim_thread_concurrent,NULL);
        } else {
           pthread_create(&g_simulation_thread,NULL,gpgpu_sim_thread_sequential,NULL);
        }
    }
}

void print_simulation_time()
{
   time_t current_time, difference, d, h, m, s;
   current_time = time((time_t *)NULL);
   difference = MAX(current_time - g_simulation_starttime, 1);

   d = difference/(3600*24);
   h = difference/3600 - 24*d;
   m = difference/60 - 60*(h + 24*d);
   s = difference - 60*(m + 60*(h + 24*d));

   fflush(stderr);
   printf("\n\ngpgpu_simulation_time = %u days, %u hrs, %u min, %u sec (%u sec)\n",
          (unsigned)d, (unsigned)h, (unsigned)m, (unsigned)s, (unsigned)difference );
   printf("gpgpu_simulation_rate = %u (inst/sec)\n", (unsigned)(g_the_gpu->gpu_tot_sim_insn / difference) );
   printf("gpgpu_simulation_rate = %u (cycle/sec)\n", (unsigned)(gpu_tot_sim_cycle / difference) );
   fflush(stdout);
}

int gpgpu_opencl_ptx_sim_main_perf( kernel_info_t *grid )
{
   g_the_gpu->launch(grid);
   sem_post(&g_sim_signal_start);
   sem_wait(&g_sim_signal_finish);
   return 0;
}

int gpgpu_opencl_ptx_sim_main_func( kernel_info_t *grid )
{
   printf("GPGPU-Sim PTX API: OpenCL functional-only simulation not yet implemented (use performance simulation)\n");
   exit(1);
}
