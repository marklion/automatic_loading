#if !defined(_LIDAR_IMP_H_)
#define _LIDAR_IMP_H_

#include "../gen_code/cpp/lidar_idl_types.h"
#include "../gen_code/cpp/lidar_service.h"
#include "../../public/lib/ad_rpc.h"
#include "../../log/lib/log_lib.h"
#include <mutex>
#include <rs_driver/api/lidar_driver.hpp>
#include <rs_driver/msg/pcl_point_cloud_msg.hpp>
#include <rs_driver/utility/sync_queue.hpp>
#include <pcl/ModelCoefficients.h>

typedef pcl::PointXYZRGB myPoint;
typedef pcl::PointCloud<myPoint> myPointCloud;
typedef PointCloudT<pcl::PointXYZI> pcMsg;

struct lidar_ply_info
{
    std::string full_ply_file;
    std::string focus_ply_file;
};
struct pc_after_split
{
    myPointCloud::Ptr legal_side;
    myPointCloud::Ptr illegal_side;
    myPointCloud::Ptr content;
    pc_after_split() : legal_side(new myPointCloud), illegal_side(new myPointCloud), content(new myPointCloud)
    {
    }
};
struct pc_after_pickup
{
    myPointCloud::Ptr last;
    myPointCloud::Ptr picked;
    pc_after_pickup() : last(new myPointCloud), picked(new myPointCloud)
    {
    }
};
enum LIDAR_POS_TYPE
{
    LIDAR_POS_DROP = 0,
    LIDAR_POS_TAIL,
};
class lidar_imp;
class lidar_driver_info
{
    double m_distance = 0;
    std::recursive_mutex m_mutex;
    int m_msop_port = 0;
    int m_difop_port = 0;
    al_log::log_tool m_logger = al_log::log_tool(al_log::LOG_LIDAR);
    robosense::lidar::LidarDriver<pcMsg> *m_lidar_driver = nullptr;
    int m_lidar_index = 0;
    bool m_for_tail = false;
    std::shared_ptr<pcMsg> m_full_cloud;
    robosense::lidar::SyncQueue<std::shared_ptr<pcMsg>> m_free_cloud_queue;
    robosense::lidar::SyncQueue<std::shared_ptr<pcMsg>> m_stuffed_cloud_queue;
    myPointCloud::Ptr m_cur_cloud;
    myPointCloud::Ptr m_output_cloud;
    std::shared_ptr<pcMsg> driverGetPointCloudFromCallerCallback();
    void driverReturnPointCloudToCallerCallback(std::shared_ptr<pcMsg> msg);
    void processCloud();
    void bind_callback2driver();
    void process_msg(std::shared_ptr<pcMsg> msg);
    myPointCloud::Ptr make_cloud_by_msg(std::shared_ptr<pcMsg> _msg);
    void save_full_cloud(std::shared_ptr<pcMsg> _cloud)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_full_cloud = _cloud;
    }
    std::shared_ptr<pcMsg> get_full_cloud()
    {
        auto ret = std::make_shared<pcMsg>();
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (m_full_cloud)
        {
            pcl::copyPointCloud(*(m_full_cloud), *(ret));
        }
        return ret;
    }
    myPointCloud::Ptr pc_transform(myPointCloud::Ptr cloud);
    myPointCloud::Ptr pc_range_filter(myPointCloud::Ptr cloud);
    myPointCloud::Ptr pc_vox_filter(myPointCloud::Ptr cloud);
    void tail_get_distance(myPointCloud::Ptr _cloud);
    pc_after_split split_cloud_to_side_and_content(myPointCloud::Ptr _cloud, bool _x_plane = false);
    std::unique_ptr<pc_after_pickup> pickup_pc_from_spec_range(myPointCloud::Ptr _orig_pc);
    void split_cloud_by_pt(myPointCloud::Ptr _cloud, const std::string &_field, float _min, float _max, myPointCloud::Ptr &_cloud_filtered, myPointCloud::Ptr &_cloud_last);
    pc_after_pickup find_points_on_plane(myPointCloud::Ptr _cloud, const Eigen::Vector3f &_ax_vec, float _distance_threshold, float _angle_threshold, pcl::ModelCoefficients::Ptr &_coe, float _cluster_distance_threshold, int _cluster_require_points);
    pc_after_pickup find_max_cluster(myPointCloud::Ptr _cloud, float _cluster_distance_threshold, int _cluster_require_points);
    std::vector<myPointCloud::Ptr> cluster_plane_points(
        myPointCloud::Ptr plane_points,
        float cluster_tolerance = 0.02f, // 聚类距离容差
        int min_cluster_size = 50,       // 最小聚类大小
        int max_cluster_size = 25000     // 最大聚类大小
    );
    void put_cloud(myPointCloud::Ptr _cloud);
    void update_cur_cloud();
    void head_get_distance(myPointCloud::Ptr _cloud);
    myPointCloud::Ptr get_output_cloud();
public:
    lidar_driver_info(int _msop_port, int _difop_port) : m_msop_port(_msop_port), m_difop_port(_difop_port) {};
    void update_distance(double _dist);
    void start_driver(int _index);
    double get_distance();
    lidar_ply_info save_ply2file(const std::string &_file_tag);
    lidar_imp *m_parent = nullptr;
    bool m_need_work = true;
    static LIDAR_POS_TYPE get_type_by_index(int _index);
};

class lidar_imp : public lidar_serviceIf
{
    enum LIDAR_INDEX
    {
        LIDAR_1 = 0,
        LIDAR_2,
        LIDAR_3,
        LIDAR_COUNT,
    };
    al_log::log_tool m_logger = al_log::log_tool(al_log::LOG_LIDAR);
    lidar_params m_params;
    std::shared_ptr<lidar_driver_info> m_lidar_result[LIDAR_COUNT] = {
        std::make_shared<lidar_driver_info>(6690, 7780),
        std::make_shared<lidar_driver_info>(6691, 7781),
        std::make_shared<lidar_driver_info>(6692, 7782),
    };
    LIDAR_INDEX get_lidar_index_by_type(LIDAR_POS_TYPE _type);
    bool m_is_lidar_on = false;
public:
    virtual bool set_lidar_params(const lidar_params &params);
    virtual bool turn_on_off_lidar(const bool is_on);
    virtual void get_lidar_params(lidar_params &_return);
    void start_all_lidar_threads();
};

#endif // _LIDAR_IMP_H_
