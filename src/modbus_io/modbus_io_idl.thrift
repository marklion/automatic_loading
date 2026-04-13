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

struct pump_param{
    1: string open_device_name,
    2: string close_device_name,
    3: i32 delay_time,
}

service modbus_io_service{
    bool add_device(1:string device_name, 2:i32 channel_id, 3:bool is_output) throws (1: ad_modbus_io_gen_exp msg),
    void del_device(1:string device_name) throws (1: ad_modbus_io_gen_exp msg),
    list<modbus_device> get_all_devices() throws (1: ad_modbus_io_gen_exp msg),
    bool device_io_set(1:string device_name, 2:bool value) throws (1: ad_modbus_io_gen_exp msg),
    bool device_io_get(1:string device_name) throws (1: ad_modbus_io_gen_exp msg),
    bool set_modbus_tcp(1:modbus_tcp_config config) throws (1: ad_modbus_io_gen_exp msg),
    modbus_tcp_config get_modbus_tcp() throws (1: ad_modbus_io_gen_exp msg),
    bool active_switch(1:bool turn_on) throws (1: ad_modbus_io_gen_exp msg),
    bool is_active() throws (1: ad_modbus_io_gen_exp msg),
    void set_pump_param(1:pump_param param) throws (1: ad_modbus_io_gen_exp msg),
    pump_param get_pump_param() throws (1: ad_modbus_io_gen_exp msg),
    oneway void pump_control(1:bool turn_on),
}