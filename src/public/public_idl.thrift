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

struct watch_dog_info {
    1: string serial_dev_name,
    2: i32 baud_rate,
    3: i32 coil_addr,
}

service public_service{
    list<daemon_meta> get_all_daemon_meta() throws (1: ad_public_gen_exp msg),
    void notify_started(1:string module_name) throws (1: ad_public_gen_exp msg),
    oneway void record_health(1:string except_info, 2:string module_name),
    list<health_info> get_health_records() throws (1: ad_public_gen_exp msg),
    void set_watch_dog_param(1:watch_dog_info info, 2:bool is_clear) throws (1: ad_public_gen_exp msg),
    watch_dog_info get_watch_dog_param() throws (1: ad_public_gen_exp msg),
    oneway void active_watch_dog(),
    oneway void reset_watch_dog(),
}