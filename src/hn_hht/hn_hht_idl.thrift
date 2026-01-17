exception hn_hht_gen_exp{
    1: string msg,
}

struct hht_config_params{
    1:string app_key,
    2:string app_secret,
}

service hn_hht_service{
    bool set_params(1:hht_config_params _params) throws (1: hn_hht_gen_exp msg),
    hht_config_params get_params() throws (1: hn_hht_gen_exp msg),
    string get_order(1:string _query_content) throws (1: hn_hht_gen_exp msg),
}