/*
Example: DistributedApp

Description:
Demontration of basic usage of the DistributedApp class

Author:
Andres Cabrera 2/2018
*/

// To run this example on a distributed cluster, you first need to copy the
// built binary to the cluster's shared file system, and then run it with mpirun.
// You can use a command like:
// mpirun -n 4 -f host_file ./distributedApp
// -n 4 determines the number of precesses launched, and -f host_file
// lists the machines where things will run. It should look like:
//
// Look at http://mpitutorial.com/tutorials/mpi-hello-world/ for more details on
// running MPI applications.


#include <stdio.h>
#include "al/core/app/al_DistributedApp.hpp"
using namespace al;

float *create_rand_nums(int num_elements) {
  float *rand_nums = (float *)malloc(sizeof(float) * num_elements);
  assert(rand_nums != NULL);
  int i;
  for (i = 0; i < num_elements; i++) {
    rand_nums[i] = (rand() / (float)RAND_MAX);
  }
  return rand_nums;
}

struct SharedState {
    float value1;
    int value2;
};

class DistributedExampleApp : public DistributedApp<SharedState> {
public:

    // The simulate function is only run for the simulator
    virtual void simulate(double dt) override {
        unsigned long num_elements_per_proc = 1e6;
        float *rand_nums = NULL;
        rand_nums = create_rand_nums(num_elements_per_proc);

        // Sum the numbers locally
        float local_sum = 0;

        for (auto i = 0ul; i < num_elements_per_proc; i++) {
          local_sum += rand_nums[i];
        }
        float global_sum;
#ifdef AL_BUILD_MPI
        // Print the random numbers on each process
        printf("Local sum for process %d - %f, avg = %f\n",
               rank(), local_sum, local_sum / num_elements_per_proc);

        // Reduce all of the local sums into the global sum

        MPI_Reduce(&local_sum, &global_sum, 1, MPI_FLOAT, MPI_SUM, 0,
                   MPI_COMM_WORLD);

        // Print the result
        if (isMaster()) {
          printf("Total sum = %f, avg = %f\n", global_sum,
                 global_sum / (rank() * num_elements_per_proc));
        }
#else
    // If MPI not available, just use local sum
    global_sum = local_sum;
#endif
        state().value1 = global_sum;
        state().value2 = num_elements_per_proc;
    }

    virtual void onDraw(Graphics &g) override {
        if (role() == ROLE_RENDERER) {
            // Renderer will recieve state from simulator
            std::cout << " Rendered got : " << state().value1 << std::endl;
        }
    }
    
    virtual void onSound(AudioIOData &io) override {
        if (role() == ROLE_AUDIO) {
            // Audio will recieve state from simulator
            std::cout << " Audio got : " << state().value1 << std::endl;
        }
    }

};

int main(){
    DistributedExampleApp app;
    app.fps(1);
    app.startFPS();
    app.print();
    for (int i = 0; i < 10; i++) {
        if (app.isPrimary()) {
            std::cout << " Run " << i << " ---------------" <<std::endl;
        }
        app.simulate(0);
    }
    app.start();
    return 0;
}
