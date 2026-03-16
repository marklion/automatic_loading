#if !defined(_STATE_MACHINE_IMP_H_)
#define _STATE_MACHINE_IMP_H_

#include "../gen_code/cpp/state_machine_service.h"
#include "../gen_code/cpp/state_machine_idl_types.h"
#include "../lidar_gen_code/cpp/lidar_service.h"
#include "../lidar_gen_code/cpp/lidar_idl_types.h"
#include "../../public/lib/ad_rpc.h"
#include "../../log/lib/log_lib.h"
#include "../../public/lib/al_utils.h"
#include <memory>
#include <string>
#include "../../pid_control/lib/pid_control_lib.h"
class state_machine_imp;
enum al_action_prompt{
    AL_ACTION_FORWARD,
    AL_ACTION_REVERSE,
    AL_ACTION_STOP
};
struct pid_output_producer{
    double m_output_rate = 0;
    al_action_prompt m_cur_action = AL_ACTION_STOP;
    pid_output_producer(double _output_rate, al_action_prompt _action):m_output_rate(_output_rate), m_cur_action(_action){}
};
struct al_sm_state
{
    enum al_sm_event
    {
        AL_SM_EVENT_EMERGENCY_SHUTDOWN,
        AL_SM_EVENT_SWITCH_TO_MANUAL_MODE,
        AL_SM_EVENT_RESET_TO_INIT,
        AL_SM_EVENT_GET_READY,
        AL_SM_EVENT_VEHICLE_COME,
        AL_SM_EVENT_VEHICLE_STAY,
        AL_SM_EVENT_VEHICLE_LEAVE,
        AL_SM_EVENT_VEHICLE_DISAPPEAR,
        AL_SM_EVENT_LOAD_ACHIEVED,
        AL_SM_EVENT_LOAD_CLEAR,
        AL_SM_EVENT_REACH_FULL,
        AL_SM_EVENT_LC_READY,
    };
    state_machine_imp *m_sm = nullptr;
    std::string m_name;
    virtual void after_enter() = 0;
    virtual void before_exit() = 0;
    virtual std::unique_ptr<al_sm_state> handle_event(al_sm_event event) = 0;
    static std::string state_name(al_sm_event _event);
    virtual void make_output_metrix(std::vector<pid_output_producer> &output_vec);
    pid_output_producer get_output(int _index);
};

struct al_sm_state_working : public al_sm_state
{
    al_sm_state_working();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
    virtual void make_output_metrix(std::vector<pid_output_producer> &output_vec) override;
};

struct al_sm_state_judge : public al_sm_state
{
    int m_stable_count = 0;
    std::string m_last_ann_content;
    AD_EVENT_SC_TIMER_NODE_PTR m_judge_timer;
    al_sm_state_judge();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
};

struct al_sm_state_cleanup : public al_sm_state
{
    al_sm_state_cleanup();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
};

struct al_sm_state_ending : public al_sm_state
{
    al_sm_state_ending();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
    virtual void make_output_metrix(std::vector<pid_output_producer> &output_vec) override;
};

struct al_sm_state_init : public al_sm_state
{
    al_sm_state_init();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
};

struct al_sm_state_ready : public al_sm_state
{
    al_sm_state_ready();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
};

struct al_sm_state_emergency : public al_sm_state
{
    al_sm_state_emergency();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
};

struct al_sm_state_manual : public al_sm_state
{
    al_sm_state_manual();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
};

struct al_sm_state_begin : public al_sm_state
{
    AD_EVENT_SC_TIMER_NODE_PTR m_action_timer;
    al_sm_state_begin();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
};

struct al_sm_state_first_heap:public al_sm_state{
    al_sm_state_first_heap();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
    virtual void make_output_metrix(std::vector<pid_output_producer> &output_vec) override;
};



class state_machine_imp : public state_machine_serviceIf
{
    std::unique_ptr<al_sm_state> m_state;
    al_log::log_tool m_logger;
    double m_current_load = 0.0;
    double m_stuff_full_offset = 0.0;
    double m_vehicle_front_x = 0.0;
    double m_vehicle_tail_x = 0.0;
    std::string m_current_kit;
    vehicle_info m_vi;
    vehicle_info m_que_vi;
    lidar_params make_params_from_kit();
    std::string m_current_prompt;
    std::string m_current_video_url;
    AD_EVENT_SC_TCP_LISTEN_NODE_PTR m_listen_node;
    std::vector<AD_EVENT_SC_TCP_DATA_NODE_PTR> m_data_nodes;
    double m_side_z = 0.0;
    double m_detect_side_z = 0.0;
    long long m_last_load_check_time = al_utils::get_current_us_stamp();
    AD_EVENT_SC_TIMER_NODE_PTR m_so_pid_timer;
    std::unique_ptr<pid_control::DiscretePID> m_stuff_offset_pid;
    std::unique_ptr<pid_control::SmithPredictor> m_smith;
    std::string m_ann_content;
    int m_ann_gap;
    pid_control::FixedWindowRateCalculator m_fwrc;
public:
    state_machine_imp();
    void remove_data_node(AD_EVENT_SC_TCP_DATA_NODE_PTR _node)
    {
        auto itr = std::find(m_data_nodes.begin(), m_data_nodes.end(), _node);
        if (itr != m_data_nodes.end())
        {
            m_data_nodes.erase(itr);
        }
    }
    ~state_machine_imp();
    void deliver_msg();
    void sm_set_current_load(double load)
    {
        m_current_load = load;
        deliver_msg();
    }
    void sm_set_current_ann(const std::string &content, int gap)
    {
        sm_basic_config tmp_info;
        get_basic_config(tmp_info);
        m_ann_content = tmp_info.channel_name + "," + content;
        m_ann_gap = gap;
        deliver_msg();
    }
    std::pair<std::string, int> sm_get_current_ann()
    {
        return {m_ann_content, m_ann_gap};
    }
    void sm_start_so_pid();
    void sm_stop_so_pid();
    double sm_get_current_load() { return m_current_load; }
    void sm_set_stuff_full_offset(double offset) { m_stuff_full_offset = offset; }
    double sm_get_stuff_full_offset() { return m_stuff_full_offset; }
    void sm_set_current_kit(const std::string &kit_name) { m_current_kit = kit_name; }
    std::string sm_get_current_kit() { return m_current_kit; }
    void sm_set_vehicle_info(const vehicle_info &v_info) { m_vi = v_info; }
    vehicle_info sm_get_vehicle_info() { return m_vi; }
    vehicle_info sm_get_queueed_vehicle_info() { return m_que_vi; }
    void sm_set_queueed_vehicle_info(const vehicle_info &v_info) { m_que_vi = v_info; }
    void sm_set_vehicle_front_x(double front_x) { m_vehicle_front_x = front_x; }
    double sm_get_vehicle_front_x() { return m_vehicle_front_x; }
    void sm_set_vehicle_tail_x(double tail_x) { m_vehicle_tail_x = tail_x; }
    double sm_get_vehicle_tail_x() { return m_vehicle_tail_x; }
    void sm_fix_side_z() { m_side_z = m_detect_side_z; }
    double sm_get_side_z() { return m_side_z; }
    void sm_set_current_prompt(const std::string &prompt)
    {
        m_current_prompt = prompt;
        deliver_msg();
    }
    std::string sm_get_current_prompt() { return m_current_prompt; }
    void sm_set_current_video_url(const std::string &video_url)
    {
        m_current_video_url = video_url;
        deliver_msg();
    }
    std::string sm_get_current_video_url() { return m_current_video_url; }
    void save_cur_ply(const std::string &_ply_tag);
    virtual void emergency_shutdown();
    virtual bool switch_to_manual_mode();
    virtual bool reset_to_init();
    virtual bool apply_config_kit(const std::string &kit_name);
    virtual bool add_config_kit(const std::string &kit_name);
    virtual void del_config_kit(const std::string &kit_name);
    virtual void get_all_config_kits(std::vector<config_kit> &_return);
    virtual bool add_kit_item(const std::string &kit_name, const std::string &item_key, const std::string &item_value);
    virtual void del_kit_item(const std::string &kit_name, const std::string &item_key);
    void sm_handle_event(al_sm_state::al_sm_event event);
    virtual void get_state_machine_status(state_machine_status &_return);
    virtual void push_cur_load(const double cur_load);
    virtual void push_stuff_full_offset(const double offset);
    virtual void trigger_sm(const vehicle_info &v_info);
    virtual void push_vehicle_front_position(const double front_x);
    virtual void push_vehicle_tail_position(const double tail_x);
    virtual bool set_basic_config(const sm_basic_config &config);
    virtual void get_basic_config(sm_basic_config &_return);
    int lc_drop_revoke_control(bool _is_drop);
    void close_all_stuff_drop();
    virtual bool set_default_kit(const std::string &kit_name);
    virtual void get_default_kit(std::string &_return);
    virtual void push_side_z(const double side_y);
    virtual void cast_info_update(const std::string &prompt, const std::string &ann_content, const int32_t ann_gap);
    void prompt_ann_while_running(al_action_prompt _fs);
};

#endif // _STATE_MACHINE_IMP_H_
