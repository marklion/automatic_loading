#include "../lib/config_lib.h"
#include <iostream>
int main(int argc, char const *argv[])
{
    auto &rc = config::root_config::get_instance();
    rc.set_child("k1", "v1");
    config::config_item t1("sk1", "sv1");
    config::config_item t2("sk2", "sv2");
    rc.set_child("s1", t1);
    rc.set_child("s2", t2);
    t2();
    std::cout << rc("k1") << std::endl;
    std::cout << rc["k1"]() << std::endl;
    std::cout << rc.expend_to_string() << std::endl;
    return 0;
}
