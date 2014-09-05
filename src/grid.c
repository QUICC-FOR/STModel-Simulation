#include <stdlib.h>
#include <assert.h>

#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>

#include "grid.h"
#include "grid_cell.h"

#define THRESDIST 0.20

/* FUNCTION PROTOTYPES */

void gr_set_random_grid(Grid *grid, gsl_rng *rng);
void gr_set_mixed_grid(Grid *grid, gsl_rng *rng);
void gr_set_uniform_grid(Grid *grid, gsl_rng *rng);
void gr_set_disturb_grid(Grid *grid, double thresDist, gsl_rng *rng);

GridCell *gr_get_cell(Grid *grid, size_t x, size_t y) {

  assert(grid);

  assert(y <= grid->yDim);
  assert(x <= grid->xDim);

  // note the arrow operator: grid->yDim === (*grid).yDim
  size_t  index = grid->xDim * y + x; 
  

  // printf("Coord(%d, %d): %d \n",(int)x,(int)y,(int)index);
  return &(grid->gridData[index]);

}

void gr_set_cell(Grid *grid, State chosenState, size_t x, size_t y) {
  // pointers validation
  assert(grid);
  GridCell *cell = gr_get_cell(grid, x, y);
  *(cell->currentState) = chosenState;
}

void gr_compute_prevalence(Grid *grid, size_t x, size_t y,  NeighType neighType) {

  // pointer validation
  assert(grid);

  //  Fix number of neighbor cells
  size_t nbSize = 8;

 if(neighType == VONNE){
    nbSize = 4;  
  }
  
// init prevalence
  double count_D = 0.0, count_C = 0.0, count_T = 0.0, count_M = 0.0;
  double increment = 1 / nbSize;

  // set aside some memory for the neighbors
  State *neighborStates = malloc(nbSize * sizeof(State));
  gr_get_neighbor_states(grid, neighborStates, x, y, neighType );

  // Compute prevalence

  for (int i = 0; i < nbSize; i++) {
    switch (neighborStates[i]) {

    case TRANSITIONAL:
      count_T = count_T + increment;
      break;

    case MIXED:
      count_M = count_M + increment;
      break;

    case DECIDUOUS:
      count_D = count_D + increment;
      break;

    case CONIFEROUS:
      count_C = count_C + increment;
      break;

    default:
      break;
    }
  }

  // Stored in cell
  gr_get_cell(grid, x, y)->prevalence[CONIFEROUS] = count_C;
  gr_get_cell(grid, x, y)->prevalence[DECIDUOUS] = count_D;
  gr_get_cell(grid, x, y)->prevalence[MIXED] = count_M;
  gr_get_cell(grid, x, y)->prevalence[TRANSITIONAL] = count_T;

  free(neighborStates);
}

void gr_get_neighbor_states(Grid *grid, State *dest, size_t x, size_t y, NeighType neighType) {

  assert(y <= grid->yDim);
  assert(x <= grid->xDim);

  












// Von neumann neighboors
  dest[0] = *(gr_get_cell(grid, x, y - 1)->currentState);
  dest[1] = *(gr_get_cell(grid, x, y + 1)->currentState);
  dest[2] = *(gr_get_cell(grid, x + 1, y)->currentState);
  dest[3] = *(gr_get_cell(grid, x - 1, y)->currentState);

// Moore neighboors
  if (neighType == MOORE) {
    dest[4] = *(gr_get_cell(grid, x - 1, y + 1)->currentState);
    dest[5] = *(gr_get_cell(grid, x - 1, y - 1)->currentState);
    dest[6] = *(gr_get_cell(grid, x + 1, y + 1)->currentState);
    dest[7] = *(gr_get_cell(grid, x + 1, y - 1)->currentState);
  }

}

Grid * gr_make_grid(size_t xsize, size_t ysize, size_t numTimeSteps, GridType gridType, gsl_rng *rng) {

  int dim = xsize * ysize;
  Grid *newGrid = malloc(sizeof(Grid));

  assert(newGrid);

  newGrid->xDim = xsize;
  newGrid->yDim = ysize;

  // **Alloc memory**

  newGrid->gridData = malloc(dim * sizeof(GridCell));
  assert(newGrid->gridData);

  // **Alloc memory GridCell level**

  // for loop across all gridData and call
  for (int i = 0; i < dim; i++) {
    newGrid->gridData[i] = *(gc_make_cell(numTimeSteps));
    //newGrid->gridData[i] = gc_make_cell(numTimeSteps);
  }


  switch (gridType) {
  case RANDOM:
    gr_set_random_grid(newGrid, rng);
    break;

  case UNIFORM:
    gr_set_uniform_grid(newGrid, rng);
    break;

  case MIX:
    gr_set_mixed_grid(newGrid, rng);
    break;

  default:
    abort();
    break;
  }


  return newGrid;
}

void gr_destroy_grid(Grid *grid) {

  const size_t xsize = grid->xDim;
  const size_t ysize = grid->yDim;

  for (int x = 0; x < xsize; x++) {
    for (int y = 0; y < ysize; y++) {
     GridCell *cell = gr_get_cell(grid,x,y);
     gc_destroy_cell(cell);
     }
  }
  
  free(grid->gridData);
  free(grid);
}

/*	 	GRID INITIALIZATION FUNCTIONS		*/

void gr_set_random_grid(Grid *grid, gsl_rng *rng) {

  for (int x = 0; x < grid->xDim; x++) {
    for (int y = 0; y < grid->yDim; y++) {
      // Pickup a random state
      int rn = gsl_rng_uniform_int(rng, 3);
      // Set state based on the random value
      switch(rn){
        case CONIFEROUS:
        gr_set_cell(grid, CONIFEROUS, x, y);
        break;

        case DECIDUOUS:
        gr_set_cell(grid, DECIDUOUS, x, y);
        break;

        case  MIXED:
        gr_set_cell(grid, MIXED, x, y);
        break;
      }
    }
  }
    gr_set_disturb_grid(grid, THRESDIST, rng);
}

void gr_set_uniform_grid(Grid *grid, gsl_rng *rng) {

  assert(grid->xDim>=3);
  assert(grid->yDim>=3);

  // Get y dimension
  int ylim = grid->yDim/3;

  for (int x = 0; x < grid->xDim; x++) {
    for (int y = 0; y < grid->yDim; y++) {

      if (y < ylim) {
        gr_set_cell(grid,CONIFEROUS,x,y);
      } 

      else if (y < 2 * ylim) {
        gr_set_cell(grid,MIXED,x,y);
      } 

      else if (y  >= 2 * ylim) { 
        gr_set_cell(grid,DECIDUOUS,x,y);
      } 
      
      else {
        printf("%s \n", "State undefined...");
        abort();
      }
    }
  }
  gr_set_disturb_grid(grid, THRESDIST, rng);
}


void gr_set_mixed_grid(Grid *grid, gsl_rng *rng) {
  assert(grid->xDim>=5);
  assert(grid->yDim>=5);

  int ylim = grid->yDim/5;

  for (int x = 0; x < grid->xDim; x++) {
    for (int y = 0; y < grid->yDim; y++) {

      int rn = gsl_rng_uniform_int(rng, 2);

      if (y < ylim) {
        gr_set_cell(grid,CONIFEROUS,x,y);
      } 

      else if (y < 2 * ylim) {
          switch(rn){
            
            case 0:
            gr_set_cell(grid,CONIFEROUS,x,y);
            break;

            case 1:
            gr_set_cell(grid,MIXED,x,y);
            break;

          }
        } 

      else if (y < 3 * ylim) { 
        gr_set_cell(grid,MIXED,x,y);
      } 

      else if (y < 4 * ylim) { 
          switch(rn){
            
            case 0:
            gr_set_cell(grid,DECIDUOUS,x,y);
            break;

            case 1:
            gr_set_cell(grid,MIXED,x,y);
            break;

          }
      } 

      else if (y >= 4 * ylim) { 
        gr_set_cell(grid,DECIDUOUS,x,y);
      } 
      
      else {
        printf("%s \n", "State undefined...");
        abort();
      }
    }
  }
  gr_set_disturb_grid(grid, THRESDIST, rng);
}

void gr_set_disturb_grid(Grid *grid, double thresDist, gsl_rng *rng) {

  thresDist = THRESDIST;

  int totalCells = grid->xDim * grid->yDim; // Get total number of cells
  int numDist = totalCells *
                thresDist; // Get number of cells disturbed based on threshold

  for (int i = 0; i < numDist; i++) {
    int rxCoord = gsl_rng_uniform_int(rng, grid->xDim);
    int ryCoord = gsl_rng_uniform_int(rng, grid->yDim);
    gr_set_cell(grid, TRANSITIONAL, rxCoord, ryCoord);
  }
}

void gr_update_grid(Grid *grid) {
}

void  gr_view_grid(Grid *grid) {

    size_t ysize = grid->yDim;
    size_t xsize = grid->xDim;

    for (int y = 0; y < ysize; ++y) {
        
            
            for (int x = 0; x < xsize; ++x) {

                GridCell *cell = gr_get_cell(grid,x,y);
                char *strState= gc_get_state(cell);
                printf("| %c ", *(strState));

            }
        
        printf("|   %d", y);
        printf("\n");
    
    }

}
