exception ad_log_gen_exp{
    1: string msg,
}

struct log_level_info{
    1: i32 level_id,
    2: i32 module_id,
}

service log_service{
    oneway void log_print(1: string log_msg, 2: i32 module_id, 3: i32 level_id),
    bool set_log_file(1: string file_path) throws (1: ad_log_gen_exp log_exp),
    string get_log_file() throws (1: ad_log_gen_exp log_exp),
    void set_log_level(1: log_level_info level_info) throws (1: ad_log_gen_exp log_exp),
    void unset_log_level(1: i32 module_id) throws (1: ad_log_gen_exp log_exp),
    list<log_level_info> get_log_level() throws (1: ad_log_gen_exp log_exp),
    list<string> get_lastest_logs(1: log_level_info level_info, 2: i32 last_line_index) throws (1: ad_log_gen_exp log_exp),
    i32 get_log_cur_line_num(1: log_level_info level_info) throws (1: ad_log_gen_exp log_exp),
}