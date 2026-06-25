#include "rhc_mtoka_osci.h"

const double T = 10;
const double DT = 0.01;

void write_data_header(mtoka_osci_t *osci)
{
  int i;

  printf("t");
  for( i=1; i<=mtoka_osci_n_neuron(osci); i++ )
    printf(",x%d,v%d,y%d,c%d,s%d", i, i, i, i, i);
  printf("\n");
}

void write_data(mtoka_osci_t *osci)
{
  int i;

  printf("%lf", mtoka_osci_time(osci));
  for( i=0; i<mtoka_osci_n_neuron(osci); i++ )
    printf(",%lf,%lf,%lf,%lf,%lf",
           vec_elem(mtoka_osci_membrane_potential(osci), i),
           vec_elem(mtoka_osci_adapt_property(osci), i),
           vec_elem(mtoka_osci_firing_rate(osci), i),
           vec_elem(mtoka_osci_tonic_input(osci), i),
           vec_elem(mtoka_osci_sensory_feedback(osci), i));
  printf("\n");
}

int main(int argc, char *argv[])
{
  mtoka_osci_t osci;
  double t;

  mtoka_osci_init( &osci, 2 );
  mtoka_osci_set_mutual_inhibit_weights_list( &osci, 0, 0.0, 2.5 );
  mtoka_osci_set_mutual_inhibit_weights_list( &osci, 1, 2.5, 0.0 );
  mtoka_osci_fill_rise_time_const( &osci, 0.25 );
  mtoka_osci_fill_adapt_time_const( &osci, 0.5 );
  mtoka_osci_fill_steady_firing_rate( &osci, 2.5 );
  mtoka_osci_fill_tonic_input( &osci, 1.5 );

  write_data_header( &osci );
  write_data( &osci );
  for( t=0; t<T; t+=DT ){
    mtoka_osci_update( &osci, DT );
    write_data( &osci );
  }
  mtoka_osci_destroy( &osci );
}
