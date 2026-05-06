#include "state_machine_imp.h"
#include "../../config/lib/config_lib.h"
#include "../../modbus_io/lib/modbus_io_lib.h"
#include "../lidar_gen_code/cpp/lidar_service.h"
#include "../lidar_gen_code/cpp/lidar_idl_types.h"
#include "../../public/lib/CJsonObject.hpp"
#include "../../public/lib/al_utils.h"
#include "../../pid_control/lib/pid_control_lib.h"
#include "../plate_gate_gen_code/cpp/plate_gate_idl_types.h"
#include "../plate_gate_gen_code/cpp/plate_gate_service.h"
#include <fstream>
#include "../../drop_system/lib/ds_lib.h"

void plate_gate_call_remote(std::function<void(plate_gate_serviceClient &)> func)
{
    AD_RPC_SC::get_instance()->call_remote<plate_gate_serviceClient>(AD_RPC_PLATE_GATE_SERVER_PORT, func);
}

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
    m_sm->stop_vp();
    m_sm->clear_vp();
    m_sm->sm_set_current_prompt("自动装车");
    m_sm->sm_set_current_ann("", -1);
    m_sm->sm_set_current_video_url("");
    m_sm->sm_set_current_kit("");
    m_sm->sm_set_stuff_full_offset(100);
    m_sm->sm_set_vehicle_info(vehicle_info());
    m_sm->sm_set_vehicle_front_x(100);
    m_sm->sm_set_vehicle_tail_x(100);
    m_sm->sm_stop_so_pid();
    m_sm->close_all_stuff_drop();
    plate_gate_call_remote(
        [](plate_gate_serviceClient &client)
        {
            client.control_gate(false);
        });
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

bool al_sm_state_init::pre_process_stuff(const std::string &_stuff_name)
{
    bool ret = false;
    std::string default_kit;
    m_sm->get_default_kit(default_kit);
    if (default_kit.empty())
    {
        std::vector<config_kit> all_kits;
        m_sm->get_all_config_kits(all_kits);
        for (auto &kit : all_kits)
        {
            auto stuff_name = kit.config_items[CONFIG_ITEM_SM_CONFIG_KIT_STUFF_NAME];
            if (stuff_name == _stuff_name)
            {
                ret = true;
                break;
            }
        }
    }
    else
    {
        ret = true;
    }
    if (!ret)
    {
        m_sm->sm_set_current_prompt("物料不匹配");
        m_sm->sm_set_current_ann("物料不匹配", -1);
    }
    return ret;
}

// al_sm_state_ready 实现
al_sm_state_ready::al_sm_state_ready()
{
    m_name = "就绪";
}

void al_sm_state_ready::after_enter()
{
    m_sm->sm_set_vehicle_info(m_sm->sm_get_queueed_vehicle_info());
    m_sm->apply_config_kit(m_sm->sm_get_queueed_vehicle_info().stuff_name);
    lidar_call_remote(
        [this](lidar_serviceClient &client)
        {
            client.turn_on_off_lidar(true);
        });
    plate_gate_call_remote(
        [this](plate_gate_serviceClient &client)
        {
            client.control_gate(true);
        });

    m_sm->sm_set_current_prompt("请缓慢往前开");
    m_sm->sm_set_current_ann("请缓慢前进", 6);
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
        new_state = std::make_unique<al_sm_state_judge>();
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
    m_sm->lc_drop_revoke_control(false);
    m_sm->sm_set_current_prompt("请停车");
    m_sm->sm_set_current_ann("停车停车", -1);
    m_sm->close_all_stuff_drop();
    m_sm->mark_as_justified();
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
    m_sm->lc_drop_revoke_control(false);
    m_sm->sm_set_current_prompt("人工装车");
    m_sm->sm_set_current_ann("请等待人工装车", -1);
    m_sm->close_all_stuff_drop();
    m_sm->mark_as_justified();
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
                auto vehicle_info = sm_get_vehicle_info();
                auto cur_offset = sm_get_stuff_full_offset();
                auto cos = al_utils::double2string(cur_offset);
                auto f_dis = sm_get_vehicle_front_x();
                auto t_dis = sm_get_vehicle_tail_x();
                auto fds = al_utils::double2string(f_dis);
                auto tds = al_utils::double2string(t_dis);
                save_cur_ply(
                    vehicle_info.plate + "-" +
                    vehicle_info.stuff_name + "-" +
                    m_state->m_name + "-" +
                    cos + "-f" + fds + "-t" + tds);
            }
            auto new_state_name = m_state->m_name;
            if (new_state_name != orig_state_name)
            {
                m_logger.log_print(al_log::LOG_LEVEL_INFO, "因为[%s],状态变化：从[%s]到[%s]", al_sm_state::state_name(event).c_str(), orig_state_name.c_str(), new_state_name.c_str());
                std::string push_sm_url = "http://localhost/api/push_sm";
                neb::CJsonObject json;
                json.Add("to", new_state_name);
                json.Add("from", orig_state_name);
                json.Add("event", al_sm_state::state_name(event));
                AD_RPC_SC::get_instance()->req_http_post(push_sm_url, json.ToString());
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
    auto tmp_param = make_params_from_kit();
    _return.applied_kit = sm_get_current_kit();
    _return.is_front_dropped = tmp_param.is_front_dropping;
    _return.side_z = sm_get_side_z();
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
    update_load2vp();
}

void state_machine_imp::push_stuff_full_offset(const double offset)
{
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
        double stuff_top_z_offset = (0 - offset - sm_get_side_z());
        sm_set_stuff_full_offset(stuff_top_z_offset);
    }
}

void state_machine_imp::trigger_sm(const vehicle_info &v_info)
{
    sm_set_queueed_vehicle_info(v_info);
    if (m_state->pre_process_stuff(v_info.stuff_name))
    {
        sm_handle_event(al_sm_state::AL_SM_EVENT_GET_READY);
    }
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
    double tail_call_x = 0.0;
    try
    {
        tail_min_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_TAIL_MIN_X));
        tail_max_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_TAIL_MAX_X));
        tail_call_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_TAIL_CALL_X));
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
        else if (sm_get_vehicle_tail_x() > tail_max_x && sm_get_vehicle_tail_x() <= tail_call_x)
        {
            sm_handle_event(al_sm_state::AL_SM_EVENT_VEHICLE_OVER_FORWARD);
        }
        else if (sm_get_vehicle_tail_x() > tail_call_x)
        {
            sm_handle_event(al_sm_state::AL_SM_EVENT_VEHICLE_DISAPPEAR);
        }
        else if (sm_get_vehicle_tail_x() < tail_min_x && sm_get_vehicle_tail_x() > -100)
        {
            sm_handle_event(al_sm_state::AL_SM_EVENT_VEHICLE_GOBACK);
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
    ci.set_child(CONFIG_ITEM_SM_CONFIG_TAIL_CALL_X, std::to_string(config.tail_call_x));
    ci.set_child(CONFIG_ITEM_SM_CONFIG_CHANNEL_NAME, config.channel_name);
    ci.set_child(CONFIG_ITEM_SM_CONFIG_EMPTY_OFFSET, std::to_string(config.empty_offset));
    ci.set_child(CONFIG_ITEM_SM_CONFIG_LACK_OFFSET, std::to_string(config.lack_offset));
    ci.set_child(CONFIG_ITEM_SM_CONFIG_ALMOST_FULL_OFFSET, std::to_string(config.almost_full_offset));
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
        _return.tail_call_x = std::stod(ci(CONFIG_ITEM_SM_CONFIG_TAIL_CALL_X));
        _return.channel_name = ci(CONFIG_ITEM_SM_CONFIG_CHANNEL_NAME);
        _return.empty_offset = std::stod(ci(CONFIG_ITEM_SM_CONFIG_EMPTY_OFFSET));
        _return.lack_offset = std::stod(ci(CONFIG_ITEM_SM_CONFIG_LACK_OFFSET));
        _return.almost_full_offset = std::stod(ci(CONFIG_ITEM_SM_CONFIG_ALMOST_FULL_OFFSET));
    }
    catch (...)
    {
        m_logger.log_print(al_log::LOG_LEVEL_DEBUG, "Invalid basic config:%s", ci.expend_to_string().c_str());
    }
}

int state_machine_imp::lc_drop_revoke_control(bool _is_drop)
{
    auto &ci = config::root_config::get_instance();
    auto cur_kit = ci[CONFIG_ITEM_SM_CONFIG_KITS][sm_get_current_kit()];
    int ret = 0;

    if (cur_kit.get_key() == sm_get_current_kit())
    {
        std::string io_name;
        std::string another_io_name;
        int stay_second = 2;
        bool should_opt_lc = true;
        if (_is_drop)
        {
            io_name = cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_DROP_LC);
            another_io_name = cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_REVOKE_LC);
            stay_second = atoi(cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_DROP_LC_STAY).c_str());
            should_opt_lc = sm_need_drop_lc();
        }
        else
        {
            io_name = cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_REVOKE_LC);
            another_io_name = cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_DROP_LC);
            stay_second = atoi(cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_REVOKE_LC_STAY).c_str());
        }

        if (!io_name.empty() && should_opt_lc)
        {
            modbus_io::set_one_io(another_io_name, false, "sm");
            modbus_io::set_one_io(io_name, true, "sm");
            m_logger.log_print(al_log::LOG_LEVEL_INFO, "按下 [%s]", io_name.c_str());
            AD_RPC_SC::get_instance()->start_one_time_timer(
                stay_second,
                [io_name, this]()
                {
                    modbus_io::set_one_io(io_name, false, "sm");
                    m_logger.log_print(al_log::LOG_LEVEL_INFO, "松开 [%s]", io_name.c_str());
                });
        }
        ret = stay_second;
    }
    return ret;
}

void state_machine_imp::lc_revoke_with_callback(std::function<void()> _func)
{
    auto sec = lc_drop_revoke_control(false);
    AD_RPC_SC::get_instance()->start_one_time_timer(sec, _func);
}

void state_machine_imp::close_all_stuff_drop()
{
    auto &ci = config::root_config::get_instance();
    auto all_kits = ci[CONFIG_ITEM_SM_CONFIG_KITS].get_children();
    for (auto &kit : all_kits)
    {
        auto ds_input_dev = (*kit)(CONFIG_ITEM_SM_CONFIG_KIT_DS_INPUT_DEV);
        drop_system::call_remote_ds(
            [ds_input_dev](drop_system_serviceClient &client)
            {
                client.set_output(0, ds_input_dev);
            });
    }
}

bool state_machine_imp::set_default_kit(const std::string &kit_name)
{
    auto &ci = config::root_config::get_instance();
    ci.set_child(CONFIG_ITEM_SM_CONFIG_DEFAULT_KIT, kit_name);

    return true;
}

void state_machine_imp::get_default_kit(std::string &_return)
{
    auto &ci = config::root_config::get_instance();
    _return = ci(CONFIG_ITEM_SM_CONFIG_DEFAULT_KIT);
}

void state_machine_imp::push_side_z(const double side_z)
{
    m_detect_side_z = side_z;
}

void state_machine_imp::cast_info_update(const std::string &prompt, const std::string &ann_content, const int32_t ann_gap)
{
    sm_set_current_ann(ann_content, ann_gap);
    sm_set_current_prompt(prompt);
}

void state_machine_imp::prompt_ann_while_running(al_action_prompt _fs)
{
    std::string content = "";
    auto ann_gap = 8;
    switch (_fs)
    {
    case AL_ACTION_FORWARD:
        content = "前进一点";
        break;
    case AL_ACTION_REVERSE:
        content = "后退一点";
        break;
    case AL_ACTION_STOP:
        content = "停车停车";
        ann_gap = 40;
        break;
    default:
        break;
    }
    sm_set_current_ann(content, ann_gap);
    sm_set_current_prompt(content);
}

lidar_params state_machine_imp::make_params_from_kit()
{
    lidar_params ret;
    auto &ci = config::root_config::get_instance();
    auto &cur_kit = ci[CONFIG_ITEM_SM_CONFIG_KITS][sm_get_current_kit()];

    ret.angle_threshold = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_ANGLE_THRESHOLD]().c_str());
    ret.cluster_distance_threshold = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_CLUSTER_DISTANCE_THRESHOLD]().c_str());
    ret.tail_cluster_distance_threshold = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_TAIL_CLUSTER_DISTANCE_THRESHOLD]().c_str());
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
    ret.seg_length_req = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_SEG_LENGTH_REQ]().c_str());
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
    ret.second_range_z_min = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_SECOND_RANGE_Z_MIN]().c_str());
    ret.second_range_z_max = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_SECOND_RANGE_Z_MAX]().c_str());
    ret.shape_filter_req = atof(cur_kit[CONFIG_ITEM_SM_CONFIG_KIT_SHAPE_FILTER_REQ]().c_str());

    return ret;
}
class log_dispatch_data_node : public AD_EVENT_SC_TCP_DATA_NODE
{
    state_machine_imp *m_service_imp;

public:
    using AD_EVENT_SC_TCP_DATA_NODE::AD_EVENT_SC_TCP_DATA_NODE;
    virtual void handleRead(const unsigned char *_data, unsigned long _size)
    {
    }
    virtual void handleError()
    {
        if (m_service_imp)
        {
            m_service_imp->remove_data_node(std::static_pointer_cast<AD_EVENT_SC_TCP_DATA_NODE>(shared_from_this()));
        }
    };
    void set_service_imp(state_machine_imp *_imp)
    {
        m_service_imp = _imp;
    }
};
class SegFunction
{
    std::map<double, double> m_output_map;
    double m_min = 0;
    double m_cur_value = 0;

public:
    SegFunction(double _min) : m_min(_min) {}
    void add_seg(double _input, double _output)
    {
        m_output_map[_input] = _output;
    }
    double update(double _input)
    {
        double target = m_min;
        for (const auto &seg : m_output_map)
        {
            if (_input >= seg.first)
            {
                target = seg.second;
            }
            else
            {
                break;
            }
        }
        m_cur_value = target;
        return target;
    }
    double cur_value()
    {
        return m_cur_value;
    }
};

state_machine_imp::state_machine_imp() : m_state(std::make_unique<al_sm_state_init>()), m_logger(al_log::LOG_STATE_MACHINE), m_fwrc(10)
{
    m_listen_node.reset(
        std::make_unique<AD_EVENT_SC_TCP_LISTEN_NODE>(
            AD_BUSINESS_PROMPT_SERVER_PORT,
            [this](int _fd, AD_EVENT_SC_TCP_LISTEN_NODE_PTR _listen_node)
            {
                auto data_node = std::make_shared<log_dispatch_data_node>(_fd, _listen_node);
                m_data_nodes.push_back(data_node);
                data_node->set_service_imp(this);
                return data_node;
            },
            AD_RPC_SC::get_instance())
            .release());
    AD_RPC_SC::get_instance()->registerNode(m_listen_node);
    m_state->m_sm = this;
    m_state->after_enter();
}

void state_machine_imp::clear_vp()
{
    m_vp.m_begin_time = "";
    m_vp.m_end_time = "";
    m_vp.m_plate = "";
    m_vp.m_dev_name = "";
    m_vp.m_load = 0;
    m_vp.m_justified = false;
}
void state_machine_imp::start_vp()
{
    m_vp.m_begin_time = al_utils::ad_utils_date_time().m_datetime_ms;
    m_vp.m_plate = sm_get_vehicle_info().plate;
    m_vp.m_dev_name = sm_get_current_kit();
}
void state_machine_imp::stop_vp()
{
    m_vp.m_end_time = al_utils::ad_utils_date_time().m_datetime_ms;
    if (!m_vp.m_plate.empty())
    {
        al_record::record_vehicle_pass(m_vp);
    }
}
void state_machine_imp::update_load2vp()
{
    auto current_load = sm_get_current_load();
    if (current_load > m_vp.m_load)
    {
        m_vp.m_load = current_load;
    }
}
void state_machine_imp::mark_as_justified()
{
    m_vp.m_justified = true;
}
state_machine_imp::~state_machine_imp()
{
    AD_RPC_SC::get_instance()->unregisterNode(m_listen_node);
}

void state_machine_imp::deliver_msg()
{
    neb::CJsonObject output;
    output.Add("url", sm_get_current_video_url());
    output.Add("prompt", sm_get_current_prompt());
    output.Add("plate", sm_get_vehicle_info().plate + "|" + sm_get_vehicle_info().stuff_name);
    output.Add("weight", al_utils::double2string(sm_get_current_load()) + " 吨");
    neb::CJsonObject ann_obj;
    auto ann_info = sm_get_current_ann();
    ann_obj.Add("content", ann_info.first);
    ann_obj.Add("gap", ann_info.second);
    output.Add("ann", ann_obj);

    auto content = output.ToString();
    content += "\n";
    for (auto &node : m_data_nodes)
    {
        send(node->getFd(), content.c_str(), content.size(), MSG_DONTWAIT);
    }
}

void state_machine_imp::sm_start_so_pid()
{
    m_stuff_offset_pid = std::make_unique<pid_control::DiscretePID>(1, 0, 0, 0, 10, 0.08);
    m_last_offset = sm_get_stuff_full_offset();
    m_offset_change_speed = 0;
    m_weight_stay_count = 0;
    m_last_weight = sm_get_current_load();
    m_so_pid_timer = AD_RPC_SC::get_instance()->startTimer(
        0,
        80,
        [this]()
        {
            bool stay_count_need_increase = false;
            if (sm_get_current_load() - m_last_weight < 0.02)
            {
                stay_count_need_increase = true;
            }
            m_last_weight = sm_get_current_load();
            // 1. 获取测量值
            auto measured_offset = sm_get_stuff_full_offset();
            measured_offset += m_state->output_offset();
            m_offset_change_speed = (measured_offset - m_last_offset) / 0.08;
            m_last_offset = measured_offset;
            std::string one_record;
            one_record =
                al_utils::ad_utils_date_time().m_datetime_ms + "," +
                std::to_string(measured_offset) + "," +
                std::to_string(m_offset_change_speed);
            sm_basic_config basic_config;
            get_basic_config(basic_config);

            // 2. 获取期望值并计算输出值
            auto expected_offset = basic_config.max_full_offset;
            auto output = m_stuff_offset_pid->execute_continuous(measured_offset, expected_offset);
            // 3. 将输出按照配置分段
            SegFunction sf(0);
            sf.add_seg(basic_config.almost_full_offset, 1);
            sf.add_seg(basic_config.lack_offset, 2);
            sf.add_seg(basic_config.empty_offset, 3);
            auto sf_output = (int)sf.update(output);
            if (sf_output == 2 && (basic_config.max_load - sm_get_current_load()) < 1)
            {
                sf_output = 1;
            }
            else
            {
                if (sf_output == 0 || sf_output == 1)
                {
                    sm_handle_event(al_sm_state::AL_SM_EVENT_REACH_FULL);
                }
                else if (sf_output == 3 && m_offset_change_speed < -12)
                {
                    sm_handle_event(al_sm_state::AL_SM_EVENT_EXCEPTION_EMPTY);
                }
            }

            // 4. 用当前状态的输出处理矩阵处理上步分段
            auto state_pid_output = m_state->get_output(sf_output);
            prompt_ann_while_running(state_pid_output.m_cur_action);
            auto &ci = config::root_config::get_instance();
            auto cur_kit = ci[CONFIG_ITEM_SM_CONFIG_KITS][sm_get_current_kit()];
            auto ds_input_dev = cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_DS_INPUT_DEV);
            if (state_pid_output.m_output_rate > 0 && stay_count_need_increase)
            {
                m_weight_stay_count++;
            }
            else
            {
                m_weight_stay_count = 0;
            }
            if (m_weight_stay_count > 140)
            {
                sm_handle_event(al_sm_state::AL_SM_EVENT_NO_STUFF);
            }

            drop_system::call_remote_ds(
                [&](drop_system_serviceClient &client)
                {
                    client.set_output(state_pid_output.m_output_rate, ds_input_dev);
                });
            one_record +=
                "," + al_utils::double2string(state_pid_output.m_output_rate);
            std::ofstream ofs("/database/pid_real_info.csv", std::ios::app);
            ofs << one_record << std::endl;
        });
}

void state_machine_imp::sm_stop_so_pid()
{
    if (m_so_pid_timer)
    {
        AD_RPC_SC::get_instance()->stopTimer(m_so_pid_timer);
    }
    m_stuff_offset_pid.reset();
}

bool state_machine_imp::sm_need_drop_lc()
{
    bool ret = true;
    if (sm_get_side_z() > -0.6)
    {
        ret = false;
    }

    return ret;
}

void state_machine_imp::save_cur_ply(const std::string &_ply_tag)
{
    AD_RPC_SC::get_instance()->add_co(
        [_ply_tag]()
        {
            lidar_call_remote(
                [_ply_tag](lidar_serviceClient &_client)
                {
                    ply_file_info ply_info;
                    _client.cap_current_ply(ply_info, _ply_tag);
                });
        });
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

bool state_machine_imp::apply_config_kit(const std::string &_stuff_name)
{
    std::string default_kit;
    get_default_kit(default_kit);
    if (default_kit.empty())
    {
        std::vector<config_kit> all_kits;
        get_all_config_kits(all_kits);
        for (auto &kit : all_kits)
        {
            auto stuff_name = kit.config_items[CONFIG_ITEM_SM_CONFIG_KIT_STUFF_NAME];
            if (stuff_name == _stuff_name)
            {
                sm_set_current_kit(kit.kit_name);
                break;
            }
        }
    }
    else
    {
        sm_set_current_kit(default_kit);
    }
    bool ret = false;
    if (!sm_get_current_kit().empty())
    {
        ret = true;
        lidar_call_remote(
            [this](lidar_serviceClient &client)
            {
                client.set_lidar_params(make_params_from_kit());
            });
        auto &ci = config::root_config::get_instance();
        auto &cur_kit = ci[CONFIG_ITEM_SM_CONFIG_KITS][sm_get_current_kit()];
        auto video_name = cur_kit(CONFIG_ITEM_SM_CONFIG_KIT_VIDEO_NAME);
        if (video_name.length() > 0)
        {
            sm_set_current_video_url("/live_rtc/" + video_name);
        }
    }
    else
    {
        ret = false;
        m_logger.log_print(al_log::LOG_LEVEL_WARN, "No config kit matched for stuff name [%s]", _stuff_name.c_str());
    }

    return ret;
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
    m_sm->sm_start_so_pid();
}

void al_sm_state_working::before_exit()
{
    m_sm->sm_stop_so_pid();
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
    case AL_SM_EVENT_NO_STUFF:
        new_state = std::make_unique<al_sm_state_manual>();
        break;
    case AL_SM_EVENT_VEHICLE_LEAVE:
        new_state = std::make_unique<al_sm_state_ending>();
        break;
    case AL_SM_EVENT_LOAD_ACHIEVED:
        new_state = std::make_unique<al_sm_state_cleanup>();
        break;
    case AL_SM_EVENT_EXCEPTION_EMPTY:
        new_state = std::make_unique<al_sm_state_first_heap>();
        break;
    case AL_SM_EVENT_VEHICLE_OVER_FORWARD:
        new_state = std::make_unique<al_sm_state_callback>();
        break;
    default:
        break;
    }
    return new_state;
}

void al_sm_state_working::make_output_matrix(std::vector<pid_output_producer> &output_vec)
{
    output_vec.push_back(pid_output_producer(0, AL_ACTION_FORWARD));
    output_vec.push_back(pid_output_producer(0.75, AL_ACTION_FORWARD));
    output_vec.push_back(pid_output_producer(1, AL_ACTION_STOP));
    output_vec.push_back(pid_output_producer(0, AL_ACTION_REVERSE));
}

al_sm_state_cleanup::al_sm_state_cleanup()
{
    m_name = "清理";
}

void al_sm_state_cleanup::after_enter()
{
    std::string wait_msg = "请等待";
    std::string leave_msg = "装车结束,请驶离";
    m_sm->sm_set_current_prompt(wait_msg);
    m_sm->sm_set_current_ann(wait_msg, -1);
    m_sm->lc_revoke_with_callback(
        [this, leave_msg]()
        {
            m_sm->sm_set_current_prompt(leave_msg);
            m_sm->sm_set_current_ann(leave_msg, -1);
        });
    m_sm->close_all_stuff_drop();
    plate_gate_call_remote(
        [](plate_gate_serviceClient &client)
        {
            client.control_gate(false);
        });
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
    m_sm->sm_start_so_pid();
}

void al_sm_state_ending::before_exit()
{
    m_sm->sm_stop_so_pid();
}

std::unique_ptr<al_sm_state> al_sm_state_ending::handle_event(al_sm_event event)
{
    auto &ci = config::root_config::get_instance();
    auto max_load_str = ci(CONFIG_ITEM_SM_CONFIG_MAX_LOAD);
    double max_load = atof(max_load_str.c_str());
    std::unique_ptr<al_sm_state> new_state;
    switch (event)
    {
    case AL_SM_EVENT_VEHICLE_DISAPPEAR:
    case AL_SM_EVENT_EMERGENCY_SHUTDOWN:
        new_state = std::make_unique<al_sm_state_emergency>();
        break;
    case AL_SM_EVENT_SWITCH_TO_MANUAL_MODE:
    case AL_SM_EVENT_NO_STUFF:
        new_state = std::make_unique<al_sm_state_manual>();
        break;
    case AL_SM_EVENT_REACH_FULL:
        if (m_sm->sm_get_current_load() < max_load)
        {
            auto tail_max_x = atof(ci(CONFIG_ITEM_SM_CONFIG_TAIL_MAX_X).c_str());
            auto tail_call_x = atof(ci(CONFIG_ITEM_SM_CONFIG_TAIL_CALL_X).c_str());
            if (m_sm->sm_get_vehicle_tail_x() > tail_max_x && m_sm->sm_get_vehicle_tail_x() <= tail_call_x)
            {
                new_state = std::make_unique<al_sm_state_callback>();
            }
            else
            {
                new_state = std::make_unique<al_sm_state_tail_stable>();
            }
        }
        else
        {
            new_state = std::make_unique<al_sm_state_cleanup>();
        }
        break;
    case AL_SM_EVENT_LOAD_ACHIEVED:
        if (m_sm->sm_get_current_load() < max_load)
        {
            new_state = std::make_unique<al_sm_state_manual>();
        }
        else
        {
            new_state = std::make_unique<al_sm_state_cleanup>();
        }
        break;
    case AL_SM_EVENT_VEHICLE_OVER_FORWARD:
        new_state = std::make_unique<al_sm_state_callback>();
        break;
    default:
        break;
    }
    return new_state;
}

void al_sm_state_ending::make_output_matrix(std::vector<pid_output_producer> &output_vec)
{
    output_vec.push_back(pid_output_producer(0, AL_ACTION_FORWARD));
    output_vec.push_back(pid_output_producer(0.75, AL_ACTION_STOP));
    output_vec.push_back(pid_output_producer(0.75, AL_ACTION_STOP));
    output_vec.push_back(pid_output_producer(0, AL_ACTION_REVERSE));
}

double al_sm_state_ending::output_offset()
{
    return -0.15;
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
    case AL_SM_EVENT_VEHICLE_STAY:
        ret = "车辆停留";
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
    case AL_SM_EVENT_REACH_FULL:
        ret = "料位达满";
        break;
    case AL_SM_EVENT_LOAD_CLEAR:
        ret = "装载量清零";
        break;
    case AL_SM_EVENT_LC_READY:
        ret = "溜槽就绪";
        break;
    case AL_SM_EVENT_EXCEPTION_EMPTY:
        ret = "异常空车";
        break;
    case AL_SM_EVENT_NO_STUFF:
        ret = "无货";
        break;
    case AL_SM_EVENT_VEHICLE_OVER_FORWARD:
        ret = "车辆前移过度";
        break;
    case AL_SM_EVENT_VEHICLE_GOBACK:
        ret = "车辆后退过度";
        break;
    case AL_SM_EVENT_VEHICLE_TAIL_STABLE:
        ret = "已稳定";
        break;
    default:
        ret = "未知事件";
        break;
    }

    return ret;
}

void al_sm_state::make_output_matrix(std::vector<pid_output_producer> &output_vec)
{
}

pid_output_producer al_sm_state::get_output(int _index)
{
    std::vector<pid_output_producer> output_vec;
    make_output_matrix(output_vec);
    if (_index >= 0 && _index < output_vec.size())
    {
        return output_vec[_index];
    }
    else
    {
        return pid_output_producer(0, AL_ACTION_STOP);
    }
}

al_sm_state_begin::al_sm_state_begin()
{
    m_name = "即将开始";
}

void al_sm_state_begin::after_enter()
{
    m_sm->sm_set_current_prompt("停车到位");
    m_sm->sm_set_current_ann("停车到位", -1);
    auto stay_second = m_sm->lc_drop_revoke_control(true);
    if (m_action_timer)
    {
        AD_RPC_SC::get_instance()->stopTimer(m_action_timer);
        m_action_timer.reset();
    }
    m_action_timer = AD_RPC_SC::get_instance()->startTimer(
        stay_second,
        [this]()
        {
            m_sm->sm_handle_event(al_sm_state::AL_SM_EVENT_LC_READY);
        });
}

void al_sm_state_begin::before_exit()
{
    if (m_action_timer)
    {
        AD_RPC_SC::get_instance()->stopTimer(m_action_timer);
        m_action_timer.reset();
    }
}

std::unique_ptr<al_sm_state> al_sm_state_begin::handle_event(al_sm_event event)
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
    case AL_SM_EVENT_LC_READY:
        new_state = std::make_unique<al_sm_state_first_heap>();
        break;
    default:
        break;
    }
    return new_state;
}

al_sm_state_judge::al_sm_state_judge()
{
    m_name = "判定";
}

void al_sm_state_judge::after_enter()
{
    m_sm->start_vp();
    m_judge_timer = AD_RPC_SC::get_instance()->startTimer(
        0, 125,
        [&]()
        {
            auto curr_hp = m_sm->sm_get_vehicle_front_x();
            sm_basic_config basic_config;
            m_sm->get_basic_config(basic_config);
            std::string ann_content;
            double gap = 0;
            if (m_is_enter)
            {
                gap = 0.05;
            }
            double ann_gap = -1;
            if (curr_hp > basic_config.front_min_x - gap && curr_hp < basic_config.front_max_x + 2 * gap)
            {
                m_sm->sm_set_current_prompt("请停车等待");
                m_stable_count++;
                ann_content = "停车停车";
                m_is_enter = true;
            }
            else if (curr_hp < basic_config.front_min_x - gap)
            {
                auto distance = basic_config.front_min_x + (basic_config.front_max_x - basic_config.front_min_x) / 2 - curr_hp;
                m_sm->sm_set_current_prompt("请前进 " + al_utils::double2string(distance) + " 米");
                m_stable_count = 0;
                ann_content = "前进前进";
                m_is_enter = false;
                ann_gap = 8;
            }
            else if (curr_hp > basic_config.front_max_x + 2 * gap)
            {
                auto distance = curr_hp - basic_config.front_max_x + (basic_config.front_max_x - basic_config.front_min_x) / 2;
                m_sm->sm_set_current_prompt("请后退 " + al_utils::double2string(distance) + " 米");
                m_stable_count = 0;
                ann_content = "后退后退";
                m_is_enter = false;
                ann_gap = 8;
            }
            if (m_stable_count >= 40)
            {
                m_sm->sm_set_current_ann("", -1);
                m_sm->sm_handle_event(al_sm_state::AL_SM_EVENT_VEHICLE_STAY);
            }
            else if (ann_content != m_last_ann_content)
            {
                m_sm->sm_set_current_ann(ann_content, ann_gap);
            }
            if (!m_is_enter && m_last_enter)
            {
                m_sm->save_cur_ply(m_sm->sm_get_vehicle_info().plate + "_" + al_utils::ad_utils_date_time().m_datetime_ms + "_judge");
            }
            m_last_enter = m_is_enter;
            m_last_ann_content = ann_content;
        });
}

void al_sm_state_judge::before_exit()
{
    AD_RPC_SC::get_instance()->stopTimer(m_judge_timer);
    m_sm->sm_fix_side_z();
}

std::unique_ptr<al_sm_state> al_sm_state_judge::handle_event(al_sm_event event)
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
    case AL_SM_EVENT_VEHICLE_STAY:
        new_state = std::make_unique<al_sm_state_begin>();
        break;
    default:
        break;
    }
    return new_state;
}

al_sm_state_first_heap::al_sm_state_first_heap()
{
    m_name = "首堆";
}

void al_sm_state_first_heap::after_enter()
{
    m_sm->sm_start_so_pid();
}

void al_sm_state_first_heap::before_exit()
{
    m_sm->sm_stop_so_pid();
}

std::unique_ptr<al_sm_state> al_sm_state_first_heap::handle_event(al_sm_event event)
{
    std::unique_ptr<al_sm_state> new_state;
    switch (event)
    {
    case AL_SM_EVENT_EMERGENCY_SHUTDOWN:
    case AL_SM_EVENT_VEHICLE_DISAPPEAR:
        new_state = std::make_unique<al_sm_state_emergency>();
        break;
    case AL_SM_EVENT_SWITCH_TO_MANUAL_MODE:
    case AL_SM_EVENT_NO_STUFF:
        new_state = std::make_unique<al_sm_state_manual>();
        break;
    case AL_SM_EVENT_VEHICLE_LEAVE:
        new_state = std::make_unique<al_sm_state_ending>();
        break;
    case AL_SM_EVENT_LOAD_ACHIEVED:
        new_state = std::make_unique<al_sm_state_cleanup>();
        break;
    case AL_SM_EVENT_REACH_FULL:
        new_state = std::make_unique<al_sm_state_working>();
        break;
    case AL_SM_EVENT_VEHICLE_OVER_FORWARD:
        new_state = std::make_unique<al_sm_state_callback>();
        break;
    default:
        break;
    }
    return new_state;
}

void al_sm_state_first_heap::make_output_matrix(std::vector<pid_output_producer> &output_vec)
{
    output_vec.push_back(pid_output_producer(0, AL_ACTION_FORWARD));
    output_vec.push_back(pid_output_producer(0.75, AL_ACTION_FORWARD));
    output_vec.push_back(pid_output_producer(1, AL_ACTION_STOP));
    output_vec.push_back(pid_output_producer(1, AL_ACTION_STOP));
}

double al_sm_state_first_heap::output_offset()
{
    return -0.05;
}

al_sm_state_callback::al_sm_state_callback()
{
    m_name = "叫回";
}

void al_sm_state_callback::after_enter()
{
    m_sm->sm_set_current_prompt("请倒车");
    m_sm->sm_set_current_ann("后退后退", -1);
    m_sm->close_all_stuff_drop();
}

void al_sm_state_callback::before_exit()
{
}

std::unique_ptr<al_sm_state> al_sm_state_callback::handle_event(al_sm_event event)
{
    std::unique_ptr<al_sm_state> new_state;

    switch (event)
    {
    case AL_SM_EVENT_VEHICLE_LEAVE:
    case AL_SM_EVENT_VEHICLE_GOBACK:
        new_state = std::make_unique<al_sm_state_ending>();
        break;
    case AL_SM_EVENT_LOAD_ACHIEVED:
        new_state = std::make_unique<al_sm_state_cleanup>();
        break;
    case AL_SM_EVENT_VEHICLE_DISAPPEAR:
    case AL_SM_EVENT_EMERGENCY_SHUTDOWN:
        new_state = std::make_unique<al_sm_state_emergency>();
        break;
    case AL_SM_EVENT_SWITCH_TO_MANUAL_MODE:
        new_state = std::make_unique<al_sm_state_manual>();
        break;
    default:
        break;
    }

    return new_state;
}

al_sm_state_tail_stable::al_sm_state_tail_stable()
{
    m_name = "尾段消抖";
}

void al_sm_state_tail_stable::after_enter()
{
    m_sm->sm_set_current_prompt("请等待");
    m_sm->sm_set_current_ann("请等待", -1);
    m_sm->close_all_stuff_drop();
    AD_RPC_SC::get_instance()->start_one_time_timer(
        7,
        [&]()
        {
            m_sm->sm_handle_event(al_sm_state::AL_SM_EVENT_VEHICLE_TAIL_STABLE);
        });
}

void al_sm_state_tail_stable::before_exit()
{
}

std::unique_ptr<al_sm_state> al_sm_state_tail_stable::handle_event(al_sm_event event)
{
    std::unique_ptr<al_sm_state> ret;
    auto &ci = config::root_config::get_instance();
    auto max_load_str = ci(CONFIG_ITEM_SM_CONFIG_MAX_LOAD);
    double max_load = atof(max_load_str.c_str());
    switch (event)
    {
    case al_sm_state::AL_SM_EVENT_VEHICLE_TAIL_STABLE:
        if (m_sm->sm_get_current_load() < max_load)
        {
            ret = std::make_unique<al_sm_state_manual>();
        }
        else
        {
            ret = std::make_unique<al_sm_state_cleanup>();
        }
        break;
    case AL_SM_EVENT_EMERGENCY_SHUTDOWN:
        ret = std::make_unique<al_sm_state_emergency>();
        break;
    case AL_SM_EVENT_SWITCH_TO_MANUAL_MODE:
        ret = std::make_unique<al_sm_state_manual>();
        break;
    default:
        break;
    }

    return ret;
}
