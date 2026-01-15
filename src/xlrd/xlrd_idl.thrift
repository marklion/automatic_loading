exception ad_xlrd_gen_exp{
    1: string msg,
}

struct xlrd_config_params{
    1:string ip,
    2:i32 port,
    3:i32 slave_id,
    4:double distance_offset,
    5:double bottom_z,
}

service xlrd_service{
    bool set_config_params(1:bool _is_front, 2:xlrd_config_params _params) throws (1: ad_xlrd_gen_exp msg),
    xlrd_config_params get_config_params(1:bool _is_front) throws (1: ad_xlrd_gen_exp msg),
    double read_distance(1:bool _is_front) throws (1: ad_xlrd_gen_exp msg),
}