#include "../lib/record_lib.h"
int main(int argc, char const *argv[])
{
    auto plate = argv[1];
    auto dev_name = argv[2];
    auto begin_time = argv[3];
    auto end_time = argv[4];
    double load = atof(argv[5]);
    al_record::vehicle_pass_record record(plate, begin_time, end_time, dev_name, load);
    al_record::record_vehicle_pass(record);
    return 0;
}
