#include <ctype.h>
#include "rhc_cmd.h"
#include "rhc_ctrl.h"
#include "rhc_ctrl_raibert.h"
#include "rhc_logger.h"
#include "rhc_model.h"
#include "rhc_simulator.h"
#include "rhc_string.h"
#include "rhc_vec.h"

#define DT 0.001
#define T  5

enum raibert_presets {
  raibert_presets_custom = 0,
  raibert_presets_full_nonlinear,
  raibert_presets_simplified_nonlinear,
  raibert_presets_simplified_nonlinear_limping,
  raibert_presets_full_linear,
  raibert_presets_tuned_simplified_linear,
  raibert_presets_untuned_simplified_linear,
};

cmd_t cmd;
model_t model;
ctrl_t ctrl;
logger_t logger;
simulator_t sim;
enum ctrl_raibert_types type_id = none;
enum raibert_presets preset_id = raibert_presets_custom;
vec_t zh_list;

void usage(int argc, const char *argv[])
{
  fprintf( stderr, "Usage:\n" );
  fprintf( stderr, "  %s <preset index>\n", argv[0] );
  fprintf( stderr, "  %s <ctrl type> <parameter>...\n", argv[0] );
  fprintf( stderr, "\n" );
  fprintf( stderr, "  List of parameter presets:\n" );
  fprintf( stderr, "    1 : Complete nonlinear spring model (Fig. 12)\n" );
  fprintf( stderr, "    2 : Simplified nonlinear spring model (Fig. 11)\n" );
  fprintf( stderr, "    3 : A limping gait: simplified nonlinear spring model (Fig. 13)\n" );
  fprintf( stderr, "    4 : Full linear spring model (Fig. 8)\n" );
  fprintf( stderr, "    5 : Tuned simplified linear spring model (Fig. 9)\n" );
  fprintf( stderr, "    6 : Untuned simplified linear spring model (Fig. 10)\n" );
  fprintf( stderr, "\n" );
  fprintf( stderr, "  List of controller types and required parameters:\n" );
  fprintf( stderr, "    full_nonlinear | fn\n" );
  fprintf( stderr, "      $ %s fn <zh> <zl> <za> <zb> <delta> <tau> <gamma> <yeta1> <g> <mu>\n", argv[0] );
  fprintf( stderr, "    simplified_nonlinear | sn\n" );
  fprintf( stderr, "      $ %s sn <zh> <zl> <za> <zb> <tau> <yeta1> <g>\n", argv[0] );
  fprintf( stderr, "    full_linear | fl\n" );
  fprintf( stderr, "      $ %s fl <zh> <zl> <za> <zb> <delta> <tau> <gamma> <yeta1> <g> <zr> <mu>\n", argv[0] );
  fprintf( stderr, "    simplified_linear | sl\n" );
  fprintf( stderr, "      $ %s sl <zh> <zl> <za> <zb> <delta> <tau> <gamma> <yeta1> <g> <zr>\n", argv[0] );
}

void parse(int argc, const char *argv[])
{
  int n_required_params;

  /* Print usage and exit */
  if( argc < 2 ||
      string_ends_with( argv[1], "h" ) ||
      string_ends_with( argv[1], "help" ) ){
    usage( argc, argv );
    exit( 0 );
  }

  /* Specify preset index */
  if( isdigit( argv[1][0] ) ){
    switch( atoi( argv[1] ) ){
      case 1:
        preset_id = raibert_presets_full_nonlinear;
        type_id = full_nonlinear;
        break;
      case 2:
        preset_id = raibert_presets_simplified_nonlinear;
        type_id = simplified_nonlinear;
        break;
      case 3:
        preset_id = raibert_presets_simplified_nonlinear_limping;
        type_id = simplified_nonlinear;
        break;
      case 4:
        preset_id = raibert_presets_full_linear;
        type_id = full_linear;
        break;
      case 5:
        preset_id = raibert_presets_tuned_simplified_linear;
        type_id = simplified_linear;
        break;
      case 6:
        preset_id = raibert_presets_untuned_simplified_linear;
        type_id = simplified_linear;
        break;
      default:
        fprintf( stderr, "Unsupported preset index: %s\n", argv[1] );
        usage( argc, argv );
        exit( 1 );
    }
  }

  /* Set custom parameters */
  if( preset_id == raibert_presets_custom ){
    if( strcmp( argv[1], "fn" ) == 0 || \
        strcmp( argv[1], "full_nonlinear" ) == 0 ){
      type_id = full_nonlinear;
      n_required_params = 12;
    } else if( strcmp( argv[1], "sn" ) == 0 || \
               strcmp( argv[1], "simplified_nonlinear" ) == 0 ){
      type_id = simplified_nonlinear;
      n_required_params = 9;
    } else if( strcmp( argv[1], "fl" ) == 0 || \
               strcmp( argv[1], "full_linear" ) == 0 ){
      type_id = full_linear;
      n_required_params = 13;
    } else if( strcmp( argv[1], "sl" ) == 0 || \
               strcmp( argv[1], "simplified_linear" ) == 0 ){
      type_id = simplified_linear;
      n_required_params = 12;
    } else {
      fprintf( stderr, "Unsupported controller type: %s\n", argv[1] );
      usage( argc, argv );
      exit( 1 );
    }
    if( n_required_params != argc ){
      fprintf( stderr, "Wrong number of parameters required for <%s>.\n", argv[1] );
      fprintf( stderr, "Requires %d arguments, but %d given.\n", n_required_params, argc );
      usage( argc, argv );
      exit( 1 );
    }
  }
}

void init()
{
  cmd_init( &cmd );
  model_init( &model, 1 );
  logger_init( &logger );
  logger_open( &logger, NULL );
  simulator_init( &sim, &cmd, &ctrl, &model );
  simulator_set_default_logger( &sim, &logger );
}

void setup_ctrl(const char *argv[])
{
  if( preset_id == raibert_presets_custom ){
    zh_list = vec_create_list( 1, atof( argv[2] ) );
    cmd.zh = atof( argv[3] );
    cmd.za = atof( argv[4] );
    cmd.zb = atof( argv[5] );
    ctrl_raibert_create( &ctrl, &cmd, &model, type_id );
    if( type_id == full_nonlinear ){
      ctrl_raibert_delta(&ctrl)        = atof( argv[6] );
      ctrl_raibert_tau(&ctrl)          = atof( argv[7] );
      ctrl_raibert_gamma(&ctrl)        = atof( argv[8] );
      ctrl_raibert_yeta1(&ctrl)        = atof( argv[9] );
      model_gravity(ctrl_model(&ctrl)) = atof( argv[10] );
      ctrl_raibert_mu(&ctrl)           = atof( argv[11] );
    } else if( type_id == simplified_nonlinear ){
      ctrl_raibert_tau(&ctrl)          = atof( argv[6] );
      ctrl_raibert_yeta1(&ctrl)        = atof( argv[7] );
      model_gravity(ctrl_model(&ctrl)) = atof( argv[8] );
    } else if( type_id == full_linear ){
      ctrl_raibert_delta(&ctrl)        = atof( argv[6] );
      ctrl_raibert_tau(&ctrl)          = atof( argv[7] );
      ctrl_raibert_gamma(&ctrl)        = atof( argv[8] );
      ctrl_raibert_yeta1(&ctrl)        = atof( argv[9] );
      model_gravity(ctrl_model(&ctrl)) = atof( argv[10] );
      ctrl_raibert_zr(&ctrl)           = atof( argv[11] );
      ctrl_raibert_mu(&ctrl)           = atof( argv[12] );
    } else if( type_id == simplified_linear ){
      ctrl_raibert_delta(&ctrl)        = atof( argv[6] );
      ctrl_raibert_tau(&ctrl)          = atof( argv[7] );
      ctrl_raibert_gamma(&ctrl)        = atof( argv[8] );
      ctrl_raibert_yeta1(&ctrl)        = atof( argv[9] );
      model_gravity(ctrl_model(&ctrl)) = atof( argv[10] );
      ctrl_raibert_zr(&ctrl)           = atof( argv[11] );
    }
  } else {
    cmd.zh = 0.5;
    cmd.za = 1.0;
    cmd.zb = 0.2;
    ctrl_raibert_create( &ctrl, &cmd, &model, type_id );
    if( preset_id == raibert_presets_full_nonlinear ){
      ctrl_raibert_set_params_full_nonlinear( &ctrl, 0.01, 41.86, 2.33, 5.81, 1 );
      model_set_gravity( ctrl_model(&ctrl), 10 );
      zh_list = vec_create_list( 2, 0.8, 0.9 );
    } else if( preset_id == raibert_presets_simplified_nonlinear ){
      ctrl_raibert_set_params_simplified_nonlinear( &ctrl, 41.86, 5.81 );
      model_set_gravity( ctrl_model(&ctrl), 10 );
      zh_list = vec_create_list( 2, 1.15, 1.4 );
    } else if( preset_id == raibert_presets_simplified_nonlinear_limping ){
      ctrl_raibert_set_params_simplified_nonlinear( &ctrl, 41.86, 2.33 );
      model_set_gravity( ctrl_model(&ctrl), 10 );
      zh_list = vec_create_list( 1, 0.02 );
    } else if( preset_id == raibert_presets_full_linear ){
      ctrl_raibert_set_params_full_linear( &ctrl, 0.05, 41.86, 2.33, 46.5, 1, 1 );
      model_set_gravity( ctrl_model(&ctrl), 10 );
      zh_list = vec_create_list( 2, 0.91, 0.98 );
    } else if( preset_id == raibert_presets_tuned_simplified_linear ){
      cmd.zh = 0;
      cmd.za = 0.5;
      cmd.zb = -0.3;
      ctrl_raibert_set_params_simplified_linear( &ctrl, 0.039, 41.86, 0.58, 46.5, 0.1 );
      model_set_gravity( ctrl_model(&ctrl), 5 );
      zh_list = vec_create_list( 2, 0.44, 0.5 );
    } else if( preset_id == raibert_presets_untuned_simplified_linear ){
      cmd.zh = 0;
      cmd.za = 0.1;
      cmd.zb = -0.2;
      ctrl_raibert_set_params_simplified_linear( &ctrl, 0.05, 41.86, 2.33, 46.5, 0.215 );
      model_set_gravity( ctrl_model(&ctrl), 10 );
      zh_list = vec_create_list( 2, -0.16, -0.18 );
    }
  }
}

void run()
{
  vec_t p0;
  register size_t i;

  p0 = vec_create( 2 );
  for( i=0; i<vec_size( zh_list ); i++ ){
    vec_set_elem_list( p0, vec_elem(zh_list, i), 0.0 );
    simulator_run( &sim, p0, T, DT, &logger, NULL );
  }
  vec_destroy( p0 );
}

void destroy()
{
  vec_destroy( zh_list );
  simulator_destroy( &sim );
  logger_destroy( &logger );
  ctrl_destroy( &ctrl );
  model_destroy( &model );
  cmd_destroy( &cmd );
}

int main(int argc, const char *argv[])
{
  parse( argc, argv );
  init();
  setup_ctrl( argv );
  run();
  destroy();
  return 0;
}
