#include "common_cli.h"
#include <fstream>

std::string common_cli::check_params(const std::vector<std::string> &_params, uint32_t _index, const std::string &_prompt)
{
    std::string ret;
    if (_params.size() <= _index)
    {
        ret = "第" + std::to_string(_index + 1) + "个参数无效，要求传入" + _prompt + "\n";
    }

    return ret;
}
