exception drop_system_gen_exp{
    1: string msg,
}

struct ds_param_info{
    1:string ip,
    2:i32 port,
    3:string device_name,
    4:double min_value,
    5:double max_value,
    6:i32 slave_id,
}

struct ds_readout{
    1:double value,
    2:double rate,
}

struct ds_input_output{
    1:string input_device_name,
    2:string output_on_device_name,
    3:string output_off_device_name,
}

service drop_system_service{
    bool add_param(1:ds_param_info param_info) throws (1: drop_system_gen_exp msg),
    void del_param(1:string device_name) throws (1: drop_system_gen_exp msg),
    list<ds_param_info> get_all_params() throws (1: drop_system_gen_exp msg),
    ds_readout readout(1:string device_name) throws (1: drop_system_gen_exp msg),
    bool add_output_match(1:string input_device_name, 2:ds_input_output output_match) throws (1: drop_system_gen_exp msg),
    void del_output_match(1:string input_device_name) throws (1: drop_system_gen_exp msg),
    list<ds_input_output> get_all_output_match() throws (1: drop_system_gen_exp msg),
    void set_output(1:double expect_rate, 2:string input_device_name) throws (1: drop_system_gen_exp msg),
    void turn_on_off(1:bool on) throws (1: drop_system_gen_exp msg),
    bool is_turned_on() throws (1: drop_system_gen_exp msg),
    bool is_moved_by_pid(1:string input_device_name) throws (1: drop_system_gen_exp msg),
}