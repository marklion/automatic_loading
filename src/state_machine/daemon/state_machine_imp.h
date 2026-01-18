#if !defined(_STATE_MACHINE_IMP_H_)
#define _STATE_MACHINE_IMP_H_

#include "../gen_code/cpp/state_machine_service.h"
#include "../gen_code/cpp/state_machine_idl_types.h"
#include "../lidar_gen_code/cpp/lidar_service.h"
#include "../lidar_gen_code/cpp/lidar_idl_types.h"
#include "../../public/lib/ad_rpc.h"
#include "../../log/lib/log_lib.h"
#include <memory>
#include <string>
class state_machine_imp;
struct al_sm_state{
    enum al_sm_event{
        AL_SM_EVENT_EMERGENCY_SHUTDOWN,
        AL_SM_EVENT_SWITCH_TO_MANUAL_MODE,
        AL_SM_EVENT_RESET_TO_INIT,
        AL_SM_EVENT_GET_READY,
        AL_SM_EVENT_VEHICLE_COME,
        AL_SM_EVENT_VEHICLE_LEAVE,
        AL_SM_EVENT_VEHICLE_DISAPPEAR,
        AL_SM_EVENT_LOAD_ACHIEVED,
        AL_SM_EVENT_LOAD_CLEAR,
        AL_SM_EVENT_REACH_FULL,
        AL_SM_EVENT_BACK_TO_EMPTY,
        AL_SM_EVENT_LC_READY,
    };
    state_machine_imp *m_sm = nullptr;
    std::string m_name;
    virtual void after_enter() = 0;
    virtual void before_exit() = 0;
    virtual std::unique_ptr<al_sm_state> handle_event(al_sm_event event) = 0;
    static std::string state_name(al_sm_event _event);
};

struct al_sm_state_working: public al_sm_state
{
    al_sm_state_working();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
};

struct al_sm_state_cleanup: public al_sm_state
{
    al_sm_state_cleanup();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
};

struct al_sm_state_ending: public al_sm_state
{
    al_sm_state_ending();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
};

struct al_sm_state_pause: public al_sm_state
{
    al_sm_state_pause();
    void after_enter() override;
    void before_exit() override;
    std::unique_ptr<al_sm_state> handle_event(al_sm_event event) override;
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
    lidar_params make_params_from_kit();
public:
    state_machine_imp();
    void sm_set_current_load(double load){m_current_load = load;}
    double sm_get_current_load(){return m_current_load;}
    void sm_set_stuff_full_offset(double offset){m_stuff_full_offset = offset;}
    double sm_get_stuff_full_offset(){return m_stuff_full_offset;}
    void sm_set_current_kit(const std::string &kit_name){m_current_kit = kit_name;}
    std::string sm_get_current_kit(){return m_current_kit;}
    void sm_set_vehicle_info(const vehicle_info &v_info){m_vi = v_info;}
    vehicle_info sm_get_vehicle_info(){return m_vi;}
    void sm_set_vehicle_front_x(double front_x){m_vehicle_front_x = front_x;}
    double sm_get_vehicle_front_x(){return m_vehicle_front_x;}
    void sm_set_vehicle_tail_x(double tail_x){m_vehicle_tail_x = tail_x;}
    double sm_get_vehicle_tail_x(){return m_vehicle_tail_x;}
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
    void drop_stuff_control(bool _is_open);
    int lc_drop_revoke_control(bool _is_drop);
};

#endif // _STATE_MACHINE_IMP_H_
