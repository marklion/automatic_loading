#include "HCNetSDK.h"
#include <string>
#include <string.h>
#include <unistd.h>
#include <memory>

std::unique_ptr<NET_DVR_PLAYCOND> make_hk_cond(const std::string &_begin_time, const std::string &_end_time, int _channel)
{
    auto cond = std::make_unique<NET_DVR_PLAYCOND>();
    cond->dwChannel = _channel;
    sscanf(_begin_time.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &cond->struStartTime.dwYear, &cond->struStartTime.dwMonth, &cond->struStartTime.dwDay, &cond->struStartTime.dwHour, &cond->struStartTime.dwMinute, &cond->struStartTime.dwSecond);
    sscanf(_end_time.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &cond->struStopTime.dwYear, &cond->struStopTime.dwMonth, &cond->struStopTime.dwDay, &cond->struStopTime.dwHour, &cond->struStopTime.dwMinute, &cond->struStopTime.dwSecond);
    cond->byDrawFrame = 0;
    cond->byStreamType = 0;
    memset(cond->byStreamID, 0, sizeof(cond->byStreamID));
    cond->byCourseFile = 0;
    cond->byDownload = 1;
    cond->byOptimalStreamType = 1;
    cond->byVODFileType = 0;
    memset(cond->byRes, 0, sizeof(cond->byRes));
    return cond;
}
void call_hk_make_video(const std::string &_ip, const std::string &_username, const std::string &_password, int _channel, const std::string &_begin_time, const std::string &_end_time, const std::string &_file_name)
{
    NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
    struLoginInfo.bUseAsynLogin = 0;                    // 同步登录方式
    strcpy(struLoginInfo.sDeviceAddress, _ip.c_str());  // 设备IP地址
    struLoginInfo.wPort = 8000;                         // 设备服务端口
    strcpy(struLoginInfo.sUserName, _username.c_str()); // 设备登录用户名
    strcpy(struLoginInfo.sPassword, _password.c_str()); // 设备登录密码
    NET_DVR_DEVICEINFO_V40 stDeviceInfoV40 = {0};

    auto lUserID = NET_DVR_Login_V40(&struLoginInfo, &stDeviceInfoV40);
    if (lUserID >= 0)
    {
        auto hk_cond = make_hk_cond(_begin_time, _end_time, _channel + stDeviceInfoV40.struDeviceV30.byStartDChan);
        std::string file_name = "/database/video/" + _file_name;
        char buff[256] = {0};
        strncpy(buff, file_name.c_str(), sizeof(buff) - 1);
        auto hPlayback = NET_DVR_GetFileByTime_V40(lUserID, buff, hk_cond.get());
        if (hPlayback >= 0)
        {
            auto start_resp = NET_DVR_PlayBackControl_V40(hPlayback, NET_DVR_PLAYSTART, NULL, 0, NULL, NULL);
            if (start_resp)
            {
                for (auto nPos = 0; nPos < 100 && nPos >= 0; nPos = NET_DVR_GetDownloadPos(hPlayback))
                {
                    usleep(50000);
                }
            }
            NET_DVR_StopGetFile(hPlayback);
        }
        NET_DVR_Logout_V30(lUserID);
    }
}

int main(int argc, char const *argv[])
{
    NET_DVR_Init();
    NET_DVR_SetConnectTime(2000, 1);
    NET_DVR_SetReconnect(10000, true);
    call_hk_make_video(argv[1], argv[2], argv[3], atoi(argv[4]), argv[5], argv[6], argv[7]);
    NET_DVR_Cleanup();
    return 0;
}
