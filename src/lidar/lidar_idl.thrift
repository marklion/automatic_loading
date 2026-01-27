exception ad_lidar_gen_exp{
    1: string msg,
}

struct lidar_params{
    1: bool is_front_dropping,
    2: double head_trans0_0,
    3: double head_trans0_1,
    4: double head_trans0_2,
    5: double head_trans0_3,
    6: double head_trans1_0,
    7: double head_trans1_1,
    8: double head_trans1_2,
    9: double head_trans1_3,
    10: double head_trans2_0,
    11: double head_trans2_1,
    12: double head_trans2_2,
    13: double head_trans2_3,
    14: double tail_trans0_0,
    15: double tail_trans0_1,
    16: double tail_trans0_2,
    17: double tail_trans0_3,
    18: double tail_trans1_0,
    19: double tail_trans1_1,
    20: double tail_trans1_2,
    21: double tail_trans1_3,
    22: double tail_trans2_0,
    23: double tail_trans2_1,
    24: double tail_trans2_2,
    25: double tail_trans2_3,
    26: double first_range_x_min,
    27: double first_range_x_max,
    28: double first_range_y_min,
    29: double first_range_y_max,
    30: double first_range_z_min,
    31: double first_range_z_max,
    32: double first_range_i_min,
    33: double first_range_i_max,
    34: double second_range_x_min,
    35: double second_range_x_max,
    36: double second_range_y_min,
    37: double second_range_y_max,
    38: double plane_distance_threshold,
    39: double angle_threshold,
    40: double voxel_leaf_size,
    41: double cluster_distance_threshold,
    42: i32 cluster_required_point_num,
    43: double seg_length_req,
    44: double second_range_z_min,
    45: double second_range_z_max,
    46: double tail_cluster_distance_threshold,
}

struct ply_file_info{
    1: string drop_file_path,
    2: string drop_full_file_path,
    3: string tail_file_path,
    4: string tail_full_file_path,
}

struct run_result{
    1: double distance,
    2: double side_z,
    3: string file_name,
}

service lidar_service{
    bool set_lidar_params(1: lidar_params params) throws (1: ad_lidar_gen_exp lidar_exp),
    lidar_params get_lidar_params() throws (1: ad_lidar_gen_exp lidar_exp),
    bool turn_on_off_lidar(1: bool is_on) throws (1: ad_lidar_gen_exp lidar_exp),
    ply_file_info cap_current_ply(1:string ply_tag) throws (1: ad_lidar_gen_exp lidar_exp),
    run_result run_against_file(1:string ply_file, 2:i32 lidar_num) throws (1: ad_lidar_gen_exp lidar_exp),
}