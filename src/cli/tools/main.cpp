#include "../../public/lib/cli.h"
#include "../../public/lib/loopscheduler.h"
#include "../../public/lib/clilocalsession.h"
#include "../../public/lib/clifilesession.h"
#include "../../public/lib/ad_rpc.h"
#include "../../public/lib/common_cli.h"
#include <fstream>
#include "../../public/lib/public_cli.h"
#include "../../log/lib/log_cli.h"
#include "../../modbus_io/lib/modbus_io_cli.h"
#include "../../state_machine/lib/state_machine_cli.h"
#include "../../public/lib/al_utils.h"
#include "../../lidar/lib/lidar_cli.h"
#include "../../xlrd/lib/xlrd_cli.h"
#include "../../live_camera/lib/live_camera_cli.h"
#include "../../hn_hht/lib/hn_hht_cli.h"

#define CLI_DEFAULT_CONFIG_FILE "/database/init.txt"

int un_safe_main(int argc, char const *argv[])
{
    common_cli *sub_c[] = {
        new public_cli(),
        new log_cli(),
        new modbus_io_cli(),
        new state_machine_cli(),
        new lidar_cli(),
        new xlrd_cli(),
        new live_camera_cli(),
        new hn_hht_cli()
    };
    auto root_menu = std::unique_ptr<cli::Menu>(new cli::Menu("ad"));
    for (auto &itr : sub_c)
    {
        root_menu->Insert(std::move(itr->menu));
    }

    auto make_bdr_string = [&]()
    {
        std::string bdr_str;
        for (auto &itr : sub_c)
        {
            bdr_str += itr->menu_name + "\n";
            bdr_str += al_utils::insert_spaces(itr->make_bdr());
            bdr_str += "ad\n";
        }
        return bdr_str;
    };

    root_menu->Insert(
        "bdr",
        [&](std::ostream &_out)
        {
            _out << make_bdr_string();
        });
    root_menu->Insert(
        "clear",
        [&](std::ostream &_out)
        {
            for (auto &itr : sub_c)
            {
                itr->clear();
            }
        });
    root_menu->Insert(
        "save",
        [&](std::ostream &_out)
        {
            std::fstream config_file(CLI_DEFAULT_CONFIG_FILE, std::ios::out | std::ios::trunc);
            if (config_file.is_open())
            {
                config_file << make_bdr_string();
                config_file.close();
                _out << "Configuration saved to " << CLI_DEFAULT_CONFIG_FILE << std::endl;
            }
            else
            {
                _out << "Failed to open configuration file for writing: " << CLI_DEFAULT_CONFIG_FILE << std::endl;
            }
        });
    root_menu->Insert(
        "bash",
        [&](std::ostream &_out)
        {
            constexpr tcflag_t ICANON_FLAG = ICANON;
            constexpr tcflag_t ECHO_FLAG = ECHO;

            termios oldt;
            termios newt;
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag |= (ICANON_FLAG | ECHO_FLAG);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
            system("/bin/bash");
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        });
    cli::LoopScheduler *plsc = nullptr;
    cli::Cli cli(std::move(root_menu));
    cli.ExitAction(
        [&](std::ostream &out)
        {
            AD_RPC_SC::get_instance()->stop_server();
            if (plsc)
            {
                plsc->Stop();
            }
        });

    auto sc = AD_RPC_SC::get_instance();
    AD_CO_ROUTINE_PTR cli_co;
    if (argc == 1)
    {
        cli::LoopScheduler lsc;
        cli::CliLocalTerminalSession ss(cli, lsc, std::cout);
        plsc = &lsc;
        cli_co = sc->add_co(
            [&]()
            {
                lsc.Run(ss);
            });

        sc->resume_co(cli_co);

        sc->start_server();
    }
    else
    {
        if (access(argv[1], F_OK) != 0)
        {
            std::cerr << "File not found: " << argv[1] << std::endl;
            return 1;
        }
        std::fstream cmd_file(argv[1], std::ios::in);
        cli::CliFileSession cf(cli, cmd_file);
        cli_co = sc->add_co(
            [&]()
            {
                cf.Start();
            });

        sc->resume_co(cli_co);

        sc->start_server();
    }

    return 0;
}
#include <iostream>
#include <security/pam_appl.h>
#include <security/pam_misc.h>

// PAM会话句柄
pam_handle_t *pamHandle = nullptr;

// 回调函数，用于获取用户名和密码
int conversation(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
{
    if (num_msg != 1)
        return PAM_CONV_ERR;

    const char *username = "root"; // 要认证的用户名
    std::string password = "";     // 用户密码
    std::cout << "Enter password: ";
    std::cin >> password;

    struct pam_response *responses = (struct pam_response *)malloc(sizeof(struct pam_response) * num_msg);
    if (responses == nullptr)
        return PAM_BUF_ERR;

    responses[0].resp = strdup(password.c_str());
    responses[0].resp_retcode = 0;
    *resp = responses;

    return PAM_SUCCESS;
}

// 主函数
int main(int argc, char const *argv[])
{
    if (argc == 1)
    {
        // 定义PAM会话的对话方式
        struct pam_conv pamConversation = {conversation, nullptr};

        // 初始化PAM会话
        int retval = pam_start("login", "root", &pamConversation, &pamHandle);
        if (retval != PAM_SUCCESS)
        {
            std::cerr << "PAM start error: " << pam_strerror(pamHandle, retval) << std::endl;
            return 1;
        }

        // 尝试用户身份认证
        retval = pam_authenticate(pamHandle, 0);
        if (retval != PAM_SUCCESS)
        {
            std::cerr << "PAM authentication error: " << pam_strerror(pamHandle, retval) << std::endl;
            pam_end(pamHandle, retval);
            return 1;
        }

        // 验证用户账户
        retval = pam_acct_mgmt(pamHandle, 0);
        if (retval != PAM_SUCCESS)
        {
            std::cerr << "PAM account management error: " << pam_strerror(pamHandle, retval) << std::endl;
            pam_end(pamHandle, retval);
            return 1;
        }

        // 认证成功，执行后续代码
        std::cout << "Authentication successful. Running post-login code..." << std::endl;

        // 在这里放置你的后续代码
        un_safe_main(argc, argv);
        // 结束PAM会话
        pam_end(pamHandle, PAM_SUCCESS);
    }
    else
    {
        un_safe_main(argc, argv);
    }

    return 0;
}
