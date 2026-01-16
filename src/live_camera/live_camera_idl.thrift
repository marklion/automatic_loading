exception ad_live_camera_gen_exp{
    1: string msg,
}

struct live_stream_config{
    1:string ip,
    2:string username,
    3:string password,
    4:string channel,
    5:string name,
}

service live_camera_service{
    bool add_live_camera(1:live_stream_config config) throws (1:ad_live_camera_gen_exp exp),
    bool del_live_camera(1:string name) throws (1:ad_live_camera_gen_exp exp),
    list<live_stream_config> get_all_live_cameras() throws (1:ad_live_camera_gen_exp exp),
}