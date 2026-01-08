exception ad_sm_gen_exp{
    1: string msg,
}

struct config_kit{
    1:string kit_name,
    2:map<string, string> config_items,
}

struct vehicle_info {
    1: string plate,
    2: string stuff_name,
}
struct state_machine_status{
    1: string status,
    2: double current_load,
    3: double stuff_full_offset,
    4: double vehicle_front_x,
    5: double vehicle_tail_x,
    6: vehicle_info v_info,
}
struct sm_basic_config{
    1: double max_load,
    2: double max_full_offset,
    3: double front_min_x,
    4: double front_max_x,
    5: double tail_min_x,
    6: double tail_max_x,
}


service state_machine_service{
    oneway void emergency_shutdown(),
    bool switch_to_manual_mode() throws (1: ad_sm_gen_exp msg),
    bool reset_to_init() throws (1: ad_sm_gen_exp msg),
    bool apply_config_kit(1: string kit_name) throws (1: ad_sm_gen_exp msg),
    bool add_config_kit(1: string kit_name) throws (1: ad_sm_gen_exp msg),
    void del_config_kit(1: string kit_name) throws (1: ad_sm_gen_exp msg),
    list<config_kit> get_all_config_kits() throws (1: ad_sm_gen_exp msg),
    bool add_kit_item(1: string kit_name, 2: string item_key, 3: string item_value) throws (1: ad_sm_gen_exp msg),
    void del_kit_item(1: string kit_name, 2: string item_key) throws (1: ad_sm_gen_exp msg),
    state_machine_status get_state_machine_status() throws (1: ad_sm_gen_exp msg),
    oneway void push_cur_load(1: double cur_load),
    oneway void push_stuff_full_offset(1: double offset),
    oneway void push_vehicle_front_position(1: double front_x),
    oneway void push_vehicle_tail_position(1: double tail_x),
    oneway void trigger_sm(1: vehicle_info v_info),
    bool set_basic_config(1: sm_basic_config config) throws (1: ad_sm_gen_exp msg),
    sm_basic_config get_basic_config() throws (1: ad_sm_gen_exp msg),
}