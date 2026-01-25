exception plate_gate_gen_exp{
    1: string msg,
}

struct plate_gate_config_params{
    1:string ip,
}

service plate_gate_service{
    bool set_params(1:plate_gate_config_params _params) throws (1: plate_gate_gen_exp msg),
    plate_gate_config_params get_params() throws (1: plate_gate_gen_exp msg),
    bool set_bypass_hht(1:bool _is_bypass) throws (1: plate_gate_gen_exp msg),
    bool get_bypass_hht() throws (1: plate_gate_gen_exp msg),
    oneway void control_gate(1:bool _is_open),
    oneway void plate_capture_notify(1:string _plate_number),
}