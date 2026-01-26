exception ad_scale_gen_exp{
    1: string msg,
}

struct scale_config_params{
    1:string dev_name,
    2:string baud_rate,
    3:double weight_coeff,
}

service scale_service{
    bool set_params(1:scale_config_params _params) throws (1: ad_scale_gen_exp msg),
    scale_config_params get_params() throws (1: ad_scale_gen_exp msg),
    double read_weight() throws (1: ad_scale_gen_exp msg),
}