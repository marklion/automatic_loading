#include "state_machine_kit_cli.h"
#include "../../config/lib/config_lib.h"
#include "state_machine_lib.h"

static void add_kit_config_cmd(cli::Menu &_menu, const std::string &_item_key, const std::string &_description)
{
    auto cmd_name = "kit_config_" + _item_key;
    _menu.Insert(
        cmd_name,
        [_item_key](std::ostream &out, std::vector<std::string> _params)
        {
            auto check_resp = common_cli::check_params(_params, 0, "请输入套件名称:");
            check_resp += common_cli::check_params(_params, 1, "请输入配置项值:");
            if (check_resp.empty())
            {
                std::string kit_name = _params[0];
                std::string item_value = _params[1];
                state_machine::call_sm_remote(
                    [kit_name, _item_key, item_value](state_machine_serviceClient &client)
                    {
                        client.add_config_kit(kit_name);
                        client.add_kit_item(kit_name, _item_key, item_value);
                    });
            }
            else
            {
                out << check_resp << std::endl;
            }
        },
        "设置套件的<" + _description + ">配置项",
        {"<kit_name> ", "<item_value>"});
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> sm_kit_menu(new cli::Menu("kit"));
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_OPEN_IO, "开始放料按钮名称");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_CLOSE_IO, "停止放料按钮名称");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_DROP_LC, "下降溜槽按钮");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_REVOKE_LC, "上升溜槽按钮");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_OPEN_IO_STAY, "开始放料按钮按下持续时间(秒)");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_CLOSE_IO_STAY, "停止放料按钮按下持续时间(秒)");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_DROP_LC_STAY, "下降溜槽按钮按下持续时间(秒)");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_REVOKE_LC_STAY, "上升溜槽按钮按下持续时间(秒)");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_IS_FRONT_DROPPING, "是否前放料(1-是,0-否)");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_0_0, "放料口雷达坐标变换矩阵00");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_0_1, "放料口雷达坐标变换矩阵01");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_0_2, "放料口雷达坐标变换矩阵02");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_0_3, "放料口雷达坐标变换矩阵03");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_1_0, "放料口雷达坐标变换矩阵10");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_1_1, "放料口雷达坐标变换矩阵11");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_1_2, "放料口雷达坐标变换矩阵12");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_1_3, "放料口雷达坐标变换矩阵13");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_2_0, "放料口雷达坐标变换矩阵20");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_2_1, "放料口雷达坐标变换矩阵21");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_2_2, "放料口雷达坐标变换矩阵22");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_HEAD_TRANS_2_3, "放料口雷达坐标变换矩阵23");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_0_0, "尾部雷达坐标变换矩阵00");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_0_1, "尾部雷达坐标变换矩阵01");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_0_2, "尾部雷达坐标变换矩阵02");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_0_3, "尾部雷达坐标变换矩阵03");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_1_0, "尾部雷达坐标变换矩阵10");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_1_1, "尾部雷达坐标变换矩阵11");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_1_2, "尾部雷达坐标变换矩阵12");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_1_3, "尾部雷达坐标变换矩阵13");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_2_0, "尾部雷达坐标变换矩阵20");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_2_1, "尾部雷达坐标变换矩阵21");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_2_2, "尾部雷达坐标变换矩阵22");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_TAIL_TRANS_2_3, "尾部雷达坐标变换矩阵23");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_ANGLE_THRESHOLD, "平面拟合角度偏差容许值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_PLANE_DISTANCE_THRESHOLD, "平面拟合距离偏差容许值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_CLUSTER_DISTANCE_THRESHOLD, "聚类距离阈值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_CLUSTER_REQUIRED_POINT_NUM, "最小聚类点数");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_I_MAX, "初步过滤强度最大值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_I_MIN, "初步过滤强度最小值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_X_MIN, "初步过滤X最小值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_X_MAX, "初步过滤X最大值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_Y_MIN, "初步过滤Y最小值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_Y_MAX, "初步过滤Y最大值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_Z_MIN, "初步过滤Z最小值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_FIRST_RANGE_Z_MAX, "初步过滤Z最大值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_SECOND_RANGE_X_MIN, "二次过滤X最小值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_SECOND_RANGE_X_MAX, "二次过滤X最大值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_SECOND_RANGE_Y_MIN, "二次过滤Y最小值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_SECOND_RANGE_Y_MAX, "二次过滤Y最大值");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_VOXEL_LEAF_SIZE, "体素滤波叶子大小");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_STUFF_NAME, "物料名称");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_VIDEO_NAME, "监控名称");

    return sm_kit_menu;
}

state_machine_kit_cli::state_machine_kit_cli() : common_cli(make_menu(), "kit")
{
}

std::string state_machine_kit_cli::make_bdr()
{
    return std::string();
}

void state_machine_kit_cli::clear()
{
}
