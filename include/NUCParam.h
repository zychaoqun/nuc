#ifndef _NUCPARAM_
#define _NUCPARAM_

#include "ros/ros.h"

#define D2R 3.14/180

class NUCParam
{
public:
    static void GetParams(ros::NodeHandle &nh);
    static double FOV;
    static double area_rotation;
    static double cx;
    static double cy;
    static double sensingTime;
    static double min_footprint;
    static std::string log_folder;
    static bool logging;

    static std::string interesting_label;
    static std::string training_set_dir;
    static int image_w;
    static int image_h;
    static std::string strategy;

    static bool simulation;
    static bool interesting_simulation;
    static bool visualization;
    static int bf_sqrt;
    static double speed;
    static double area_length;

    static bool sim_running;

    static double percent_interesting;
    static int patches;
    static bool auto_exit;
    static bool bypass_controller;

    static double int_prob_thr;
    static double alpha_h0;
    static double alpha_hm;
    static double beta_h0;
    static double beta_hm;

    static double prob_r;
    static double pathCost;

    static std::string nuc_dir;
    static std::string prior_file_name;

    static int lm_tracks;
    static int high_res_cells;
    static std::string policy;
    static double time_limit;
    static double average_speed;
    static double turning_time;

    static double random_seed;
    static double gp_sigma;

};

#endif
