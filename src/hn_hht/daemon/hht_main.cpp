#include "../gen_code/cpp/hn_hht_idl_types.h"
#include "../gen_code/cpp/hn_hht_service.h"
#include "../../public/lib/ad_rpc.h"
#include "../../public/lib/al_utils.h"
#include "../../public/lib/CJsonObject.hpp"
#include "../../log/lib/log_lib.h"
#include "../../config/lib/config_lib.h"
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <mutex>

static al_log::log_tool g_logger(al_log::LOG_HHT);
std::mutex g_mutex;

class SM4SignatureGenerator
{
private:
    static const int KEY_SIZE = 16; // 128 bits

    // 将字节数组转换为十六进制字符串
    static std::string bytesToHex(const unsigned char *data, size_t length)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < length; ++i)
        {
            ss << std::setw(2) << static_cast<int>(data[i]);
        }
        return ss.str();
    }

    // SM4加密
    static std::string sm4Encrypt(const std::string &plaintext, const unsigned char *key)
    {
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx)
        {
            g_logger.log_print(al_log::LOG_LEVEL_ERROR, "Failed to create EVP_CIPHER_CTX");
            return "";
        }

        // 设置加密
        if (EVP_EncryptInit_ex(ctx, EVP_sm4_ecb(), NULL, key, NULL) != 1)
        {
            g_logger.log_print(al_log::LOG_LEVEL_ERROR, "Failed to initialize SM4 encryption");
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }

        // 启用PKCS7填充（默认启用，确保数据长度是块大小的倍数）
        EVP_CIPHER_CTX_set_padding(ctx, 1);

        // 计算输出缓冲区大小
        int ciphertext_len = 0;
        int len = 0;
        std::vector<unsigned char> ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH);

        // 执行加密
        if (EVP_EncryptUpdate(
                ctx,
                ciphertext.data(),
                &len,
                reinterpret_cast<const unsigned char *>(plaintext.c_str()),
                plaintext.length()) != 1)
        {
            g_logger.log_print(al_log::LOG_LEVEL_ERROR, "Encryption update failed");
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        ciphertext_len = len;

        // 完成加密
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1)
        {
            g_logger.log_print(al_log::LOG_LEVEL_ERROR, "Encryption final failed");
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        ciphertext_len += len;

        EVP_CIPHER_CTX_free(ctx);

        return bytesToHex(ciphertext.data(), ciphertext_len);
    }

    // 生成SM4密钥
    static bool generateSM4Key(unsigned char *key)
    {
        return RAND_bytes(key, KEY_SIZE) == 1;
    }

    // 签名
    static std::string signature(const std::string &data)
    {
        unsigned char key[KEY_SIZE];

        // 生成随机密钥
        if (!generateSM4Key(key))
        {
            g_logger.log_print(al_log::LOG_LEVEL_ERROR, "Failed to generate SM4 key");
            return "";
        }

        // 使用SM4加密
        return sm4Encrypt(data, key);
    }

public:
    /**
     * 生成签名
     * @param appKey appKey
     * @param appSecret appSecret
     * @param timestamp 当前时间戳(毫秒)
     * @return 签名
     */
    static std::string sign(const std::string &appKey,
                            const std::string &appSecret,
                            const std::string &timestamp)
    {
        // 参数映射
        std::map<std::string, std::string> params = {
            {"appKey", appKey},
            {"appSecret", appSecret},
            {"timestamp", timestamp}};

        // 1. 参数名称按首字母排序
        std::vector<std::string> sortedKeys;
        for (const auto &pair : params)
        {
            sortedKeys.push_back(pair.first);
        }
        std::sort(sortedKeys.begin(), sortedKeys.end());

        // 2. 拼接参数值成一个字符串
        std::string dataToEncrypt;
        for (const auto &key : sortedKeys)
        {
            dataToEncrypt += params[key];
        }

        // 3. 使用 SM4 算法生成签名
        return signature(dataToEncrypt);
    }
};

class hn_hht_imp : public hn_hht_serviceIf
{
public:
    virtual bool set_params(const hht_config_params &_params)
    {
        auto &ci = config::root_config::get_instance();
        ci.set_child(CONFIG_ITEM_HHT_KEY, _params.app_key);
        ci.set_child(CONFIG_ITEM_HHT_SECRET, _params.app_secret);
        return true;
    }
    virtual void get_params(hht_config_params &_return)
    {
        auto &ci = config::root_config::get_instance();
        _return.app_key = ci(CONFIG_ITEM_HHT_KEY);
        _return.app_secret = ci(CONFIG_ITEM_HHT_SECRET);
    }
    virtual void get_order(std::string &_return, const std::string &_query_content)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto &ci = config::root_config::get_instance();
        std::string app_key = ci(CONFIG_ITEM_HHT_KEY);
        std::string app_secret = ci(CONFIG_ITEM_HHT_SECRET);
        std::string timestamp = al_utils::get_current_timestamp_ms();
        std::string signature = SM4SignatureGenerator::sign(app_key, app_secret, timestamp);
        auto content = al_utils::URLCodec::encode_query_param(_query_content);
        std::string get_req = "https://api-uat.hnjt.top/api/openapi/v1/transport/billInfo?no=" + content;
        std::string wget_cmd = "wget --no-check-certificate --quiet --method GET --timeout=15 ";
        wget_cmd += "--header 'appKey: " + app_key + "' ";
        wget_cmd += "--header 'signature: " + signature + "' ";
        wget_cmd += "--header 'timestamp: " + timestamp + "' ";
        wget_cmd += "'" + get_req + "' -O /tmp/hht_order_response.json";
        g_logger.log_print(al_log::LOG_LEVEL_INFO, "cmd length:%d", wget_cmd.length());
        g_logger.log_print(al_log::LOG_LEVEL_INFO, "Generated signature: %s", signature.c_str());
        g_logger.log_print(al_log::LOG_LEVEL_INFO, "Executing wget command: %s", wget_cmd.c_str());
        AD_RPC_SC::get_instance()->non_block_system(wget_cmd);
        AD_RPC_SC::get_instance()->yield_by_timer(0, 100);
        std::ifstream ifs("/tmp/hht_order_response.json");
        if (ifs.is_open())
        {
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            _return = buffer.str();
        }
        else
        {
            g_logger.log_print(al_log::LOG_LEVEL_ERROR, "Failed to open HHT order response file");
            _return = "";
        }
    }
};

int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    sc->enable_rpc_server(AD_RPC_HHT_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<hn_hht_serviceProcessor>(std::make_shared<hn_hht_imp>()));
    sc->start_server();
    return 0;
}
