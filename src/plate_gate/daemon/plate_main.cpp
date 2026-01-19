#include "../gen_code/cpp/plate_gate_idl_types.h"
#include "../gen_code/cpp/plate_gate_service.h"
#include "../../public/lib/ad_rpc.h"
#include "../../log/lib/log_lib.h"
#include "../../public/lib/al_utils.h"
#include "../../config/lib/config_lib.h"
#include "../../hn_hht/lib/hn_hht_lib.h"
#include "../../state_machine/lib/state_machine_lib.h"
#include "../../public/lib/CJsonObject.hpp"
extern "C"
{
#include "VzLPRClientSDK.h"
}

static void fini_plate_camera();
class plate_gate_imp;
static al_log::log_tool g_logger(al_log::LOG_PLATE_GATE);
static int g_plate_camera_handle = -1;
class plate_gate_imp : public plate_gate_serviceIf
{
public:
    virtual bool set_params(const plate_gate_config_params &_params)
    {
        auto &ci = config::root_config::get_instance();
        ci.set_child(CONFIG_ITEM_PLATE_GATE_CAMERA_IP, _params.ip);
        return true;
    }
    virtual void get_params(plate_gate_config_params &_return)
    {
        auto &ci = config::root_config::get_instance();
        _return.ip = ci(CONFIG_ITEM_PLATE_GATE_CAMERA_IP);
    }
    virtual void control_gate(const bool _is_open)
    {
        int sz_ret = -1;
        if (g_plate_camera_handle > 0)
        {
            if (_is_open)
            {
                sz_ret = VzLPRClient_SetIOOutputAuto(g_plate_camera_handle, 0, 1000);
            }
            else
            {
                sz_ret = VzLPRClient_SetIOOutputAuto(g_plate_camera_handle, 1, 1000);
            }
            if (sz_ret != 0)
            {
                g_logger.log_print(al_log::LOG_LEVEL_ERROR, "failed to control gate:%d", sz_ret);
            }
        }
    }
    virtual void plate_capture_notify(const std::string &_plate_number)
    {
        auto hht_resp = hn_hht::req_get_order(_plate_number);
        neb::CJsonObject resp_hht_json(hht_resp);
        bool hht_success = false;
        resp_hht_json.Get("success", hht_success);
        if (hht_success)
        {
            auto stuff_name = resp_hht_json["result"]("productName");
            state_machine::call_sm_remote(
                [&](state_machine_serviceClient &client)
                {
                    vehicle_info v_info;
                    v_info.plate = _plate_number;
                    v_info.stuff_name = stuff_name;
                    client.trigger_sm(v_info);
                });
        }
    }
};
static void exit_driver(const std::string &msg)
{
    g_logger.log_print(al_log::LOG_LEVEL_ERROR, "%s", msg.c_str());
    fini_plate_camera();
}

static void common_callback(VzLPRClientHandle handle, void *pUserData, VZ_LPRC_COMMON_NOTIFY eNotify, const char *pStrDetail)
{
    switch (eNotify)
    {
    case VZ_LPRC_ACCESS_DENIED:
    case VZ_LPRC_NETWORK_ERR:
    case VZ_LPRC_OFFLINE:
        exit_driver("device offline");
        break;
    case VZ_LPRC_ONLINE:
        break;
    default:
        break;
    }
}
int plate_callback(VzLPRClientHandle handle, void *pUserData,
                   const TH_PlateResult *pResult, unsigned uNumPlates,
                   VZ_LPRC_RESULT_TYPE eResultType,
                   const VZ_LPRC_IMAGE_INFO *pImgFull,
                   const VZ_LPRC_IMAGE_INFO *pImgPlateClip)
{
    std::string plate = pResult->license;
    auto p_com_driver = (plate_gate_imp *)pUserData;
    if (plate.length() > 0)
    {
        auto utf_plate = al_utils::util_gbk2utf(plate);
        if (utf_plate.find("无") == std::string::npos)
        {
            p_com_driver->plate_capture_notify(utf_plate);
        }
    }

    return 0;
}

static int init_plate_camera(const std::string &_ip, plate_gate_imp &_pmi)
{
    int zs_ret = -1;
    int handle_ret = -1;
    if (0 == (zs_ret = VZLPRClient_SetCommonNotifyCallBack(common_callback, nullptr)))
    {
        auto handler = VzLPRClient_Open(_ip.c_str(), 80, "admin", "admin");
        if (handler > 0)
        {
            if (0 == (zs_ret = VzLPRClient_SetPlateInfoCallBack(handler, plate_callback, &_pmi, false)))
            {
                g_logger.log_print(al_log::LOG_LEVEL_INFO, "plate camera connected:%s", _ip.c_str());
                handle_ret = handler;
            }
            else
            {
                exit_driver("failed to set callback:" + std::to_string(zs_ret));
            }
        }
        else
        {
            exit_driver("failed to open device:" + std::to_string(handler));
        }
    }
    else
    {
        exit_driver("failed to set common callback:" + std::to_string(zs_ret));
    }

    return handle_ret;
}
void fini_plate_camera()
{
    if (g_plate_camera_handle > 0)
    {
        VzLPRClient_Close(g_plate_camera_handle);
        g_plate_camera_handle = -1;
    }
}

int main(int argc, char const *argv[])
{
    VzLPRClient_Setup();
    auto sc = AD_RPC_SC::get_instance();
    auto pmi = std::make_shared<plate_gate_imp>();
    sc->enable_rpc_server(AD_RPC_PLATE_GATE_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<plate_gate_serviceProcessor>(pmi));
    sc->startTimer(
        5, [&]()
        {
            auto &ci = config::root_config::get_instance();
            std::string ip = ci(CONFIG_ITEM_PLATE_GATE_CAMERA_IP);
            if (ip.size() > 0 && g_plate_camera_handle < 0)
            {
                g_plate_camera_handle = init_plate_camera(ip, *pmi);
            } });
    sc->start_server();
    VzLPRClient_Cleanup();
    return 0;
}
