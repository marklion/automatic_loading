exception ad_public_gen_exp{
    1: string msg,
}

struct daemon_meta{
    1: string daemon_name,
    2: i32 pid,
    3: string start_time,
}

service public_service{
    list<daemon_meta> get_all_daemon_meta() throws (1: ad_public_gen_exp msg),
}