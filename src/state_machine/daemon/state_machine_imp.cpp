#include "state_machine_imp.h"
#include "../../config/lib/config_lib.h"
#include "../../modbus_io/lib/modbus_io_lib.h"
#include "../lidar_gen_code/cpp/lidar_service.h"
#include "../lidar_gen_code/cpp/lidar_idl_types.h"

void lidar_call_remote(std::function<void(lidar_serviceClient &)> func)
{
    AD_RPC_SC::get_instance()->call_remote<lidar_serviceClient>(AD_RPC_LIDAR_SERVER_PORT, func);
}

// al_sm_state_init 实现
al_sm_state_init::al_sm_state_init()
{
    m_name = "空闲";
}

void al_sm_state_init::after_enter()
{
    m_sm->sm_set_current_kit("");
    m_sm->sm_set_stuff_full_offset(100);
    m_sm->sm_set_vehicle_info(vehicle_info());
    m_sm->sm_set_vehicle_front_x(100);
    m_sm->sm_set_vehicle_tail_x(100);
    lidar_call_remote(
        [](lidar_serviceClient &client)
        {
            client.turn_on_off_lidar(false);
        });
}

void al_sm_state_init::before_exit()
{
}

std::unique_ptr<al_sm_state> al_sm_state_init::handle_event(al_sm_event event)
{
    std::unique_ptr<al_sm_state> new_state;
    switch (event)
    {
    case AL_SM_EVENT_EMERGENCY_SHUTDOWN:
        new_state = std::make_unique<al_sm_state_emergency>();
        break;
    case AL_SM_EVENT_SWITCH_TO_MANUAL_MODE:
        new_state = std::make_unique<al_sm_state_manual>();
        break;
    case AL_SM_EVENT_GET_READY:
        new_state = std::make_unique<al_sm_state_ready>();
        break;
    default:
        break;
    }
    return new_state;
}

// al_sm_state_ready 实现
al_sm_state_ready::al_sm_state_ready()
{
    m_name = "就绪";
}

void al_sm_state_ready::after_enter()
{
}

void al_sm_state_ready::before_exit()
{
}

std::unique_ptr<al_sm_state> al_sm_state_ready::handle_event(al_sm_event event)
{
    std::unique_ptr<al_sm_state> new_state;
    switch (event)
    {
    case AL_SM_EVENT_EMERGENCY_SHUTDOWN:
        new_state = std::make_unique<al_sm_state_emergency>();
        break;
    case AL_SM_EVENT_SWITCH_TO_MANUAL_MODE:
        new_state = std::make_unique<al_sm_state_manual>();
        break;
    case AL_SM_EVENT_VEHICLE_COME:
        new_state = std::make_unique<al_sm_state_working>();
        break;
    default:
        break;
    }
    return new_state;
}

// al_sm_state_emergency 实现
al_sm_state_emergency::al_sm_state_emergency()
{
    m_name = "急停";
}

void al_sm_state_emergency::after_enter()
{
}

void al_sm_state_emergency::before_exit()
{
}

std::unique_ptr<al_sm_state> al_sm_state_emergency::handle_event(al_sm_event event)
{
    std::unique_ptr<al_sm_state> new_state;
    switch (event)
    {
    case AL_SM_EVENT_RESET_TO_INIT:
        new_state = std::make_unique<al_sm_state_init>();
        break;
    default:
        break;
    }
    return new_state;
}

// al_sm_state_manual 实现
al_sm_state_manual::al_sm_state_manual()
{
    m_name = "手动";
}

void al_sm_state_manual::after_enter()
{
}

void al_sm_state_manual::before_exit()
{
}

std::unique_ptr<al_sm_state> al_sm_state_manual::handle_event(al_sm_event event)
{
    std::unique_ptr<al_sm_state> new_state;
    switch (event)
    {
    case AL_SM_EVENT_RESET_TO_INIT:
        new_state = std::make_unique<al_sm_state_init>();
        break;
    default:
        break;
    }
    return new_state;
}

void state_machine_imp::sm_handle_event(al_sm_state::al_sm_event event)
{
    m_logger.log_print(al_log::LOG_LEVEL_DEBUG, "Event [%s] pushed", al_sm_state::state_name(event).c_str());
    AD_RPC_SC::get_instance()->add_co(
        [this, event]()
        {
            auto orig_state_name = m_state->m_name;
            auto new_state = m_state->handle_event(event);
            if (new_state)
            {
                m_state->before_exit();
                m_state = std::move(new_state);
                m_state->m_sm = this;
                m_state->after_enter();
            }
            auto new_state_name = m_state->m_name;
            if (new_state_name != orig_state_name)
            {
                m_logger.log_print(al_log::LOG_LEVEL_INFO, "因为[%s],状态变化：从[%s]到[%s]", al_sm_state::state_name(event).c_str(), orig_state_name.c_str(), new_state_name.c_str());
            }
        });
}

void state_machine_imp::get_state_machine_status(state_machine_status &_return)
{
    _return.status = m_state->m_name;
    _return.current_load = sm_get_current_load();
    _return.stuff_full_offset = sm_get_stuff_full_offset();
    _return.v_info = sm_get_vehicle_info();
    _return.vehicle_front_x = sm_get_vehicle_front_x();
    _return.vehicle_tail_x = sm_get_vehicle_tail_x();
    get_basic_config(_return.basic_config);
}

void state_machine_imp::push_cur_load(const double cur_load)
{
    sm_set_current_load(cur_load);
    auto &ci = config::root_config::get_instance();
    auto max_load_str = ci(CONFIG_ITEM_SM_CONFIG_MAX_LOAD);
    double max_load = 0.0;
    try
    {
        max_load = std::stod(max_load_str);
    }
    catch (...)
    {
        max_load = 0.0;
        m_logger.log_print(al_log::LOG_LEVEL_DEBUG, "Invalid max_load config: %s", max_load_str.c_str());
    }
    if (max_load > 0.0 && sm_get_current_load() >= max_load)
    {
        sm_handle_event(al_sm_state::AL_SM_EVENT_LOAD_ACHIEVED);
    }
    else if (sm_get_current_load() == 0)
    {
        sm_handle_event(al_sm_state::AL_SM_EVENT_LOAD_CLEAR);
    }
}

void state_machine_imp::push_stuff_full_offset(const double offset)
{
    sm_set_stuff_full_offset(offset);
    auto &ci = config::root_config::get_instance();
    auto max_offset_str = ci(CONFIG_ITEM_SM_CONFIG_MAX_FULL_OFFSET);
    double max_offset = -10;
    try
    {
        max_offset = std::stod(max_offset_str);
    }
    catch (...)
    {
        max_offset = -10;
        m_logger.log_print(al_log::LOG_LEVEL_DEBUG, "Invalid max_full_offset config: %s", max_offset_str.c_str());
    }
    if (max_offset > -10)
    {
        if (sm_get_stuff_full_offset() >= max_offset)
        {
            sm_handle_event(al_sm_state::AL_SM_EVENT_REACH_FULL);
        }
        else
        {
            sm_handle_event(al_sm_state::AL_SM_EVENT_BACK_TO_EMPTY);
        }
    }
}

void state_machine_imp::trigger_sm(const vehicle_info &v_info)
{
    sm_set_vehicle_info(v_info);
    apply_config_kit(v_info.stuff_name);
    lidar_call_remote(
        [this](lidar_serviceClient &client)
        {
            client.turn_on_off_lidar(true);
        });
    sm_handle_event(al_sm_state::AL_SM_EVENT_GET_READY);
}

void state_machine_imp::push_vehicle_front_position(const double front_x)
{
    sm_set_vehicle_front_x(front_x);
    auto &ci = config::root_config::get_instance();
    double front_min_x = 0.0;
    double front_max_x = 0.0;
    try
    {
        front_min_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_FRONT_MIN_X));
        front_max_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_FRONT_MAX_X));
    }
    catch (...)
    {
        front_min_x = 0.0;
        front_max_x = 0.0;
        m_logger.log_print(al_log::LOG_LEVEL_DEBUG, "Invalid front_x config: min=%s, max=%s",
                           ci(CONFIG_ITEM_SM_CONFIG_FRONT_MIN_X).c_str(),
                           ci(CONFIG_ITEM_SM_CONFIG_FRONT_MAX_X).c_str());
    }
    if (front_min_x < front_max_x &&
        sm_get_vehicle_front_x() >= front_min_x &&
        sm_get_vehicle_front_x() <= front_max_x)
    {
        sm_handle_event(al_sm_state::AL_SM_EVENT_VEHICLE_COME);
    }
}

void state_machine_imp::push_vehicle_tail_position(const double tail_x)
{
    sm_set_vehicle_tail_x(tail_x);
    auto &ci = config::root_config::get_instance();
    double tail_min_x = 0.0;
    double tail_max_x = 0.0;
    try
    {
        tail_min_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_TAIL_MIN_X));
        tail_max_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_TAIL_MAX_X));
    }
    catch (...)
    {
        tail_min_x = 0.0;
        tail_max_x = 0.0;
        m_logger.log_print(al_log::LOG_LEVEL_DEBUG, "Invalid tail_x config: min=%s, max=%s",
                           ci(CONFIG_ITEM_SM_CONFIG_TAIL_MIN_X).c_str(),
                           ci(CONFIG_ITEM_SM_CONFIG_TAIL_MAX_X).c_str());
    }
    if (tail_min_x < tail_max_x)
    {
        if (sm_get_vehicle_tail_x() >= tail_min_x &&
            sm_get_vehicle_tail_x() <= tail_max_x)
        {
            sm_handle_event(al_sm_state::AL_SM_EVENT_VEHICLE_LEAVE);
        }
        else if (sm_get_vehicle_tail_x() > tail_max_x)
        {
            sm_handle_event(al_sm_state::AL_SM_EVENT_VEHICLE_DISAPPEAR);
        }
    }
}

bool state_machine_imp::set_basic_config(const sm_basic_config &config)
{
    auto &ci = config::root_config::get_instance();
    ci.set_child(CONFIG_ITEM_SM_CONFIG_MAX_LOAD, std::to_string(config.max_load));
    ci.set_child(CONFIG_ITEM_SM_CONFIG_MAX_FULL_OFFSET, std::to_string(config.max_full_offset));
    ci.set_child(CONFIG_ITEM_SM_CONFIG_FRONT_MIN_X, std::to_string(config.front_min_x));
    ci.set_child(CONFIG_ITEM_SM_CONFIG_FRONT_MAX_X, std::to_string(config.front_max_x));
    ci.set_child(CONFIG_ITEM_SM_CONFIG_TAIL_MIN_X, std::to_string(config.tail_min_x));
    ci.set_child(CONFIG_ITEM_SM_CONFIG_TAIL_MAX_X, std::to_string(config.tail_max_x));
    return true;
}

void state_machine_imp::get_basic_config(sm_basic_config &_return)
{
    auto &ci = config::root_config::get_instance();
    try
    {
        _return.max_load = std::stod(ci(CONFIG_ITEM_SM_CONFIG_MAX_LOAD));
        _return.max_full_offset = std::stod(ci(CONFIG_ITEM_SM_CONFIG_MAX_FULL_OFFSET));
        _return.front_min_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_FRONT_MIN_X));
        _return.front_max_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_FRONT_MAX_X));
        _return.tail_min_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_TAIL_MIN_X));
        _return.tail_max_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_TAIL_MAX_X));
    }
    catch (...)
    {
        m_logger.log_print(al_log::LOG_LEVEL_DEBUG, "Invalid basic config:%s", ci.expend_to_string().c_str());
    }
}

void state_machine_imp::drop_stuff_control(bool _is_open)
{
    auto &ci = config::root_config::get_instance();
    auto cur_kit = ci[CONFIG_ITEM_SM_CONFIG_KITS][sm_get_current_kit()];
    if (cur_kit.get_key() == sm_get_current_kit())
    {
        std::string io_name;
        std::string another_io_name;
        if (_is_open)
        {
            io_name = cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_OPEN_IO);
            another_io_name = cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_CLOSE_IO);
        }
        else
        {
            io_name = cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_CLOSE_IO);
            another_io_name = cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_OPEN_IO);
        }
        if (!io_name.empty())
        {
            modbus_io::set_one_io(another_io_name, false);
            modbus_io::set_one_io(io_name, true);
            m_logger.log_print(al_log::LOG_LEVEL_INFO, "按下 [%s]", io_name.c_str());
            AD_RPC_SC::get_instance()->start_one_time_timer(
                2,
                [io_name, this]()
                {
                    modbus_io::set_one_io(io_name, false);
                    m_logger.log_print(al_log::LOG_LEVEL_INFO, "松开 [%s]", io_name.c_str());
                });
        }
    }
}

lidar_params state_machine_imp::make_params_from_kit()
{
    lidar_params ret;
    auto &ci = config::root_config::get_instance();
    auto &cur_kit = ci[CONFIG_ITEM_SM_CONFIG_KITS][sm_get_current_kit()];

    ret.angle_threshold = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_ANGLE_THRESHOLD]().c_str());
    ret.cluster_distance_threshold = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_CLUSTER_DISTANCE_THRESHOLD]().c_str());
    ret.cluster_required_point_num = atoi(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_CLUSTER_REQUIRED_POINT_NUM]().c_str());
    ret.first_range_x_min = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_X_MIN]().c_str());
    ret.first_range_x_max = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_X_MAX]().c_str());
    ret.first_range_y_min = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_Y_MIN]().c_str());
    ret.first_range_y_max = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_Y_MAX]().c_str());
    ret.first_range_z_min = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_Z_MIN]().c_str());
    ret.first_range_z_max = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_Z_MAX]().c_str());
    ret.first_range_i_min = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_I_MIN]().c_str());
    ret.first_range_i_max = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_I_MAX]().c_str());
    ret.plane_distance_threshold = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_PLANE_DISTANCE_THRESHOLD]().c_str());
    ret.voxel_leaf_size = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_VOXEL_LEAF_SIZE]().c_str());
    ret.head_trans0_0 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_0_0]().c_str());
    ret.head_trans0_1 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_0_1]().c_str());
    ret.head_trans0_2 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_0_2]().c_str());
    ret.head_trans0_3 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_0_3]().c_str());
    ret.head_trans1_0 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_1_0]().c_str());
    ret.head_trans1_1 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_1_1]().c_str());
    ret.head_trans1_2 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_1_2]().c_str());
    ret.head_trans1_3 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_1_3]().c_str());
    ret.head_trans2_0 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_2_0]().c_str());
    ret.head_trans2_1 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_2_1]().c_str());
    ret.head_trans2_2 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_2_2]().c_str());
    ret.head_trans2_3 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_2_3]().c_str());
    ret.tail_trans0_0 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_0_0]().c_str());
    ret.tail_trans0_1 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_0_1]().c_str());
    ret.tail_trans0_2 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_0_2]().c_str());
    ret.tail_trans0_3 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_0_3]().c_str());
    ret.tail_trans1_0 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_1_0]().c_str());
    ret.tail_trans1_1 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_1_1]().c_str());
    ret.tail_trans1_2 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_1_2]().c_str());
    ret.tail_trans1_3 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_1_3]().c_str());
    ret.tail_trans2_0 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_2_0]().c_str());
    ret.tail_trans2_1 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_2_1]().c_str());
    ret.tail_trans2_2 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_2_2]().c_str());
    ret.tail_trans2_3 = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_2_3]().c_str());
    ret.is_front_dropping = (cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_IS_FRONT_DROPPING]() == "1");
    ret.second_range_x_max = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_SECOND_RANGE_X_MAX]().c_str());
    ret.second_range_x_min = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_SECOND_RANGE_X_MIN]().c_str());
    ret.second_range_y_max = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_SECOND_RANGE_Y_MAX]().c_str());
    ret.second_range_y_min = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_SECOND_RANGE_Y_MIN]().c_str());

    return ret;
}

state_machine_imp::state_machine_imp() : m_state(std::make_unique<al_sm_state_init>()), m_logger(al_log::LOG_STATE_MACHINE)
{
}

void state_machine_imp::emergency_shutdown()
{
    sm_handle_event(al_sm_state::AL_SM_EVENT_EMERGENCY_SHUTDOWN);
}

bool state_machine_imp::switch_to_manual_mode()
{
    sm_handle_event(al_sm_state::AL_SM_EVENT_SWITCH_TO_MANUAL_MODE);
    return true;
}

bool state_machine_imp::reset_to_init()
{
    sm_handle_event(al_sm_state::AL_SM_EVENT_RESET_TO_INIT);
    return true;
}

bool state_machine_imp::apply_config_kit(const std::string &kit_name)
{
    sm_set_current_kit(kit_name);
    lidar_call_remote(
        [this](lidar_serviceClient &client)
        {
            client.set_lidar_params(make_params_from_kit());
        });
    return true;
}

bool state_machine_imp::add_config_kit(const std::string &kit_name)
{
    auto &ci = config::root_config::get_instance();
    ci.set_child(CONFIG_ITEM_SM_CONFIG_KITS);
    auto &all_kits = ci.get_child(CONFIG_ITEM_SM_CONFIG_KITS);
    all_kits.set_child(kit_name);
    return true;
}

void state_machine_imp::del_config_kit(const std::string &kit_name)
{
    auto &ci = config::root_config::get_instance();
    ci.set_child(CONFIG_ITEM_SM_CONFIG_KITS);
    auto &all_kits = ci.get_child(CONFIG_ITEM_SM_CONFIG_KITS);
    all_kits.remove_child(kit_name);
}

void state_machine_imp::get_all_config_kits(std::vector<config_kit> &_return)
{
    auto &ci = config::root_config::get_instance();
    auto &all_kits = ci.get_child(CONFIG_ITEM_SM_CONFIG_KITS);
    if (!all_kits.is_empty())
    {
        auto children = all_kits.get_children();
        for (const auto &child : children)
        {
            config_kit kit;
            kit.kit_name = child->get_key();
            auto kit_items = child->get_children();
            for (const auto &item : kit_items)
            {
                std::map<std::string, std::string> kit_item;
                kit.config_items[item->get_key()] = item->get_value();
            }
            _return.push_back(kit);
        }
    }
}

bool state_machine_imp::add_kit_item(const std::string &kit_name, const std::string &item_key, const std::string &item_value)
{
    auto &ci = config::root_config::get_instance();
    ci.set_child(CONFIG_ITEM_SM_CONFIG_KITS);
    auto &all_kits = ci.get_child(CONFIG_ITEM_SM_CONFIG_KITS);
    all_kits.set_child(kit_name);
    auto &one_kit = all_kits.get_child(kit_name);
    one_kit.set_child(item_key, item_value);

    return true;
}

void state_machine_imp::del_kit_item(const std::string &kit_name, const std::string &item_key)
{
    auto &ci = config::root_config::get_instance();
    ci.set_child(CONFIG_ITEM_SM_CONFIG_KITS);
    auto &all_kits = ci.get_child(CONFIG_ITEM_SM_CONFIG_KITS);
    all_kits.set_child(kit_name);
    auto &one_kit = all_kits.get_child(kit_name);
    one_kit.remove_child(item_key);
}

al_sm_state_working::al_sm_state_working()
{
    m_name = "工作中";
}

void al_sm_state_working::after_enter()
{
    m_sm->drop_stuff_control(true);
}

void al_sm_state_working::before_exit()
{
    m_sm->drop_stuff_control(false);
}

std::unique_ptr<al_sm_state> al_sm_state_working::handle_event(al_sm_event event)
{
    std::unique_ptr<al_sm_state> new_state;
    switch (event)
    {
    case AL_SM_EVENT_EMERGENCY_SHUTDOWN:
    case AL_SM_EVENT_VEHICLE_DISAPPEAR:
        new_state = std::make_unique<al_sm_state_emergency>();
        break;
    case AL_SM_EVENT_SWITCH_TO_MANUAL_MODE:
        new_state = std::make_unique<al_sm_state_manual>();
        break;
    case AL_SM_EVENT_VEHICLE_LEAVE:
        new_state = std::make_unique<al_sm_state_ending>();
        break;
    case AL_SM_EVENT_LOAD_ACHIEVED:
        new_state = std::make_unique<al_sm_state_cleanup>();
        break;
    case AL_SM_EVENT_REACH_FULL:
        new_state = std::make_unique<al_sm_state_pause>();
        break;
    default:
        break;
    }
    return new_state;
}

al_sm_state_cleanup::al_sm_state_cleanup()
{
    m_name = "清理";
}

void al_sm_state_cleanup::after_enter()
{
}

void al_sm_state_cleanup::before_exit()
{
}

std::unique_ptr<al_sm_state> al_sm_state_cleanup::handle_event(al_sm_event event)
{
    std::unique_ptr<al_sm_state> new_state;
    switch (event)
    {
    case AL_SM_EVENT_EMERGENCY_SHUTDOWN:
        new_state = std::make_unique<al_sm_state_emergency>();
        break;
    case AL_SM_EVENT_SWITCH_TO_MANUAL_MODE:
        new_state = std::make_unique<al_sm_state_manual>();
        break;
    case AL_SM_EVENT_LOAD_CLEAR:
        new_state = std::make_unique<al_sm_state_init>();
        break;
    default:
        break;
    }
    return new_state;
}

al_sm_state_ending::al_sm_state_ending()
{
    m_name = "收尾";
}

void al_sm_state_ending::after_enter()
{
    m_sm->drop_stuff_control(true);
}

void al_sm_state_ending::before_exit()
{
    m_sm->drop_stuff_control(false);
}

std::unique_ptr<al_sm_state> al_sm_state_ending::handle_event(al_sm_event event)
{
    std::unique_ptr<al_sm_state> new_state;
    switch (event)
    {
    case AL_SM_EVENT_VEHICLE_DISAPPEAR:
    case AL_SM_EVENT_EMERGENCY_SHUTDOWN:
        new_state = std::make_unique<al_sm_state_emergency>();
        break;
    case AL_SM_EVENT_SWITCH_TO_MANUAL_MODE:
        new_state = std::make_unique<al_sm_state_manual>();
        break;
    case AL_SM_EVENT_REACH_FULL:
    case AL_SM_EVENT_LOAD_ACHIEVED:
        new_state = std::make_unique<al_sm_state_cleanup>();
        break;
    default:
        break;
    }
    return new_state;
}

al_sm_state_pause::al_sm_state_pause()
{
    m_name = "暂停";
}

void al_sm_state_pause::after_enter()
{
}

void al_sm_state_pause::before_exit()
{
}

std::unique_ptr<al_sm_state> al_sm_state_pause::handle_event(al_sm_event event)
{
    std::unique_ptr<al_sm_state> new_state;
    switch (event)
    {
    case AL_SM_EVENT_EMERGENCY_SHUTDOWN:
        new_state = std::make_unique<al_sm_state_emergency>();
        break;
    case AL_SM_EVENT_SWITCH_TO_MANUAL_MODE:
        new_state = std::make_unique<al_sm_state_manual>();
        break;
    case AL_SM_EVENT_VEHICLE_DISAPPEAR:
    case AL_SM_EVENT_LOAD_ACHIEVED:
        new_state = std::make_unique<al_sm_state_cleanup>();
        break;
    case AL_SM_EVENT_BACK_TO_EMPTY:
        new_state = std::make_unique<al_sm_state_working>();
        break;
    default:
        break;
    }
    return new_state;
}

std::string al_sm_state::state_name(al_sm_event _event)
{
    std::string ret;
    switch (_event)
    {
    case AL_SM_EVENT_EMERGENCY_SHUTDOWN:
        ret = "急停";
        break;
    case AL_SM_EVENT_SWITCH_TO_MANUAL_MODE:
        ret = "切换到手动模式";
        break;
    case AL_SM_EVENT_RESET_TO_INIT:
        ret = "重置到初始状态";
        break;
    case AL_SM_EVENT_GET_READY:
        ret = "触发就绪";
        break;
    case AL_SM_EVENT_VEHICLE_COME:
        ret = "车辆到达";
        break;
    case AL_SM_EVENT_VEHICLE_LEAVE:
        ret = "车辆离开";
        break;
    case AL_SM_EVENT_VEHICLE_DISAPPEAR:
        ret = "车辆消失";
        break;
    case AL_SM_EVENT_LOAD_ACHIEVED:
        ret = "达到装载量";
        break;
    case AL_SM_EVENT_LOAD_CLEAR:
        ret = "装载量清零";
        break;
    case AL_SM_EVENT_REACH_FULL:
        ret = "达到满载偏移";
        break;
    case AL_SM_EVENT_BACK_TO_EMPTY:
        ret = "回到空载偏移";
        break;
    default:
        ret = "未知事件";
        break;
    }

    return ret;
}
