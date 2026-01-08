exception ad_modbus_io_gen_exp{
    1: string msg,
}

struct modbus_device{
    1: string device_name,
    2: i32 channel_id,
    3: bool is_output,
    4: bool is_opened,
}

struct modbus_tcp_config{
    1: string host_name,
    2: i32 port,
    3: i32 device_id,
}

service modbus_io_service{
    bool add_device(1:string device_name, 2:i32 channel_id, 3:bool is_output) throws (1: ad_modbus_io_gen_exp msg),
    void del_device(1:string device_name) throws (1: ad_modbus_io_gen_exp msg),
    list<modbus_device> get_all_devices() throws (1: ad_modbus_io_gen_exp msg),
    bool device_io_set(1:string device_name, 2:bool value) throws (1: ad_modbus_io_gen_exp msg),
    bool device_io_get(1:string device_name) throws (1: ad_modbus_io_gen_exp msg),
    bool set_modbus_tcp(1:modbus_tcp_config config) throws (1: ad_modbus_io_gen_exp msg),
    modbus_tcp_config get_modbus_tcp() throws (1: ad_modbus_io_gen_exp msg),
}