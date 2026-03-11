exception ad_public_gen_exp{
    1: string msg,
}

struct daemon_meta{
    1: string daemon_name,
    2: i32 pid,
    3: string start_time,
}

struct health_info{
    1: string module_name,
    2: string except_info,
    3: string record_time,
}

service public_service{
    list<daemon_meta> get_all_daemon_meta() throws (1: ad_public_gen_exp msg),
    void notify_started(1:string module_name) throws (1: ad_public_gen_exp msg),
    oneway void record_health(1:string except_info, 2:string module_name),
    list<health_info> get_health_records() throws (1: ad_public_gen_exp msg),
}