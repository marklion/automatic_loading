#include "lidar_imp.h"
#include "../../config/lib/config_lib.h"
#include <pcl/common/transforms.h>
#include <pcl/filters/passthrough.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/common/impl/angles.hpp>
#include <pcl/filters/extract_indices.h>
#include <pcl/search/kdtree.h>
#include <pcl/segmentation/extract_clusters.h>
#include <boost/filesystem.hpp>
#include <pcl/io/ply_io.h>
#include "../../public/lib/al_utils.h"
#include "../../state_machine/lib/state_machine_lib.h"
#include <sys/fcntl.h>
#include <sys/file.h>

static void serializePointCloud(myPointCloud::Ptr cloud)
{
    int fd = open("/tmp/cloud.lock", O_CREAT | O_RDWR, 0666); // 打开或创建锁文件
    if (fd == -1)
    {
        perror("Failed to open lock file");
        return;
    }

    if (flock(fd, LOCK_EX) == -1) // 加独占锁
    {
        perror("Failed to acquire file lock");
        close(fd);
        return;
    }

    // 写入点云数据
    std::ofstream ofs("/tmp/cloud.bin", std::ios::binary);
    if (!ofs.is_open())
    {
        perror("Failed to open /tmp/cloud.bin");
        flock(fd, LOCK_UN); // 释放锁
        close(fd);
        return;
    }

    for (const auto &point : *cloud)
    {
        float data[7] = {
            point.x, point.y, point.z, point.r * 1.0f, point.g * 1.0f, point.b * 1.0f, 0.0f};
        ofs.write(reinterpret_cast<char *>(data), sizeof(data));
    }

    ofs.close();

    flock(fd, LOCK_UN); // 释放锁
    close(fd);          // 关闭文件描述符
}

lidar_imp::LIDAR_INDEX lidar_imp::get_lidar_index_by_type(LIDAR_POS_TYPE _type)
{
    LIDAR_INDEX index = LIDAR_1;
    lidar_params params;
    get_lidar_params(params);
    if (_type == LIDAR_POS_TAIL)
    {
        index = LIDAR_3;
    }
    else
    {
        if (params.is_front_dropping)
        {
            index = LIDAR_1;
        }
        else
        {
            index = LIDAR_2;
        }
    }

    return index;
}

bool lidar_imp::set_lidar_params(const lidar_params &params)
{
    m_params = params;
    return true;
}

bool lidar_imp::turn_on_off_lidar(const bool is_on)
{
    m_is_lidar_on = is_on;
    if (!is_on)
    {
        for (int i = 0; i < LIDAR_COUNT; ++i)
        {
            m_lidar_result[i]->m_need_work = is_on;
        }
    }
    else
    {
        auto drop_lidar = m_lidar_result[get_lidar_index_by_type(LIDAR_POS_DROP)];
        auto tail_lidar = m_lidar_result[get_lidar_index_by_type(LIDAR_POS_TAIL)];
        drop_lidar->m_need_work = is_on;
        tail_lidar->m_need_work = is_on;
    }

    return true;
}

void lidar_imp::get_lidar_params(lidar_params &_return)
{
    _return = m_params;
}
void lidar_imp::cap_current_ply(ply_file_info &_return, const std::string &ply_tag)
{
    bool only_focus = true;
    std::string tag = ply_tag;
    if (ply_tag.empty())
    {
        tag = "manual_save";
        only_focus = false;
    }
    auto front_tag = tag + "_drop";
    auto tail_tag = tag + "_tail";
    auto drop_lidar = m_lidar_result[get_lidar_index_by_type(LIDAR_POS_DROP)];
    auto tail_lidar = m_lidar_result[get_lidar_index_by_type(LIDAR_POS_TAIL)];
    auto drop_resp = drop_lidar->save_ply2file(front_tag, only_focus);
    auto tail_resp = tail_lidar->save_ply2file(tail_tag, only_focus);
    _return.drop_file_path = drop_resp.focus_ply_file;
    _return.drop_full_file_path = drop_resp.full_ply_file;
    _return.tail_file_path = tail_resp.focus_ply_file;
    _return.tail_full_file_path = tail_resp.full_ply_file;
}
long long get_current_us_stamp()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    long long current_useconds = tv.tv_sec * 1000000LL + tv.tv_usec;
    return current_useconds;
}

static std::unique_ptr<robosense::lidar::RSDriverParam> init_rs_param(int msop_port, int difop_port)
{
    using namespace robosense::lidar;
    std::unique_ptr<RSDriverParam> param(new RSDriverParam);
    param->decoder_param.dense_points = true;
    param->input_param.msop_port = msop_port;
    param->input_param.difop_port = difop_port;
    param->lidar_type = LidarType::RSAIRY;
    param->input_type = InputType::ONLINE_LIDAR;
    return param;
}

void lidar_imp::start_all_lidar_threads()
{
    for (int i = 0; i < LIDAR_COUNT; ++i)
    {
        m_lidar_result[i]->start_driver(i);
        m_lidar_result[i]->m_parent = this;
    }
    AD_RPC_SC::get_instance()->startTimer(
        0,
        120,
        [this]()
        {
            if (m_is_lidar_on)
            {
                auto drop_lidar = m_lidar_result[get_lidar_index_by_type(LIDAR_POS_DROP)];
                auto tail_lidar = m_lidar_result[get_lidar_index_by_type(LIDAR_POS_TAIL)];
                double drop_distance = drop_lidar->get_distance();
                double tail_distance = tail_lidar->get_distance();
                double side_z = drop_lidar->get_side_z();
                state_machine::call_sm_remote(
                    [drop_distance, tail_distance, side_z](state_machine_serviceClient &sm_client)
                    {
                        sm_client.push_vehicle_front_position(drop_distance);
                        sm_client.push_vehicle_tail_position(tail_distance);
                        sm_client.push_side_z(side_z);
                    });
                drop_lidar->serial_pc();
            }
        });
}

myPointCloud::Ptr lidar_driver_info::pc_transform(myPointCloud::Ptr cloud)
{
    Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
    lidar_params params;
    m_parent->get_lidar_params(params);
    if (!m_for_tail)
    {
        transform(0, 0) = params.head_trans0_0;
        transform(0, 1) = params.head_trans0_1;
        transform(0, 2) = params.head_trans0_2;
        transform(0, 3) = params.head_trans0_3;
        transform(1, 0) = params.head_trans1_0;
        transform(1, 1) = params.head_trans1_1;
        transform(1, 2) = params.head_trans1_2;
        transform(1, 3) = params.head_trans1_3;
        transform(2, 0) = params.head_trans2_0;
        transform(2, 1) = params.head_trans2_1;
        transform(2, 2) = params.head_trans2_2;
        transform(2, 3) = params.head_trans2_3;
        transform(3, 0) = 0;
        transform(3, 1) = 0;
        transform(3, 2) = 0;
        transform(3, 3) = 1;
    }
    else
    {
        transform(0, 0) = params.tail_trans0_0;
        ;
        transform(0, 1) = params.tail_trans0_1;
        transform(0, 2) = params.tail_trans0_2;
        transform(0, 3) = params.tail_trans0_3;
        transform(1, 0) = params.tail_trans1_0;
        transform(1, 1) = params.tail_trans1_1;
        transform(1, 2) = params.tail_trans1_2;
        transform(1, 3) = params.tail_trans1_3;
        transform(2, 0) = params.tail_trans2_0;
        transform(2, 1) = params.tail_trans2_1;
        transform(2, 2) = params.tail_trans2_2;
        transform(2, 3) = params.tail_trans2_3;
        transform(3, 0) = 0;
        transform(3, 1) = 0;
        transform(3, 2) = 0;
        transform(3, 3) = 1;
    }

    myPointCloud::Ptr ret(new myPointCloud);
    myPointCloud::Ptr tmp = cloud;

    pcl::transformPointCloud(*tmp, *ret, transform);

    return ret;
}

myPointCloud::Ptr lidar_driver_info::pc_range_filter(myPointCloud::Ptr cloud)
{
    lidar_params params;
    m_parent->get_lidar_params(params);
    auto x_min = params.first_range_x_min;
    auto x_max = params.first_range_x_max;
    auto y_min = params.first_range_y_min;
    auto y_max = params.first_range_y_max;
    auto z_min = params.first_range_z_min;
    auto z_max = params.first_range_z_max;
    auto i_min = params.first_range_i_min;
    auto i_max = params.first_range_i_max;
    myPointCloud::Ptr tmp = cloud;
    pcl::PassThrough<myPoint> pass_x;
    pass_x.setInputCloud(tmp);
    pass_x.setFilterFieldName("x");
    pass_x.setFilterLimits(x_min, x_max);
    myPointCloud::Ptr cloud_filtered_x(new myPointCloud);
    pass_x.filter(*cloud_filtered_x);

    pcl::PassThrough<myPoint> pass_y;
    pass_y.setInputCloud(cloud_filtered_x);
    pass_y.setFilterFieldName("y");
    pass_y.setFilterLimits(y_min, y_max);
    myPointCloud::Ptr cloud_filtered_y(new myPointCloud);
    pass_y.filter(*cloud_filtered_y);

    pcl::PassThrough<myPoint> pass_z;
    pass_z.setInputCloud(cloud_filtered_y);
    pass_z.setFilterFieldName("z");
    pass_z.setFilterLimits(z_min, z_max);
    myPointCloud::Ptr cloud_filtered_z(new myPointCloud);
    pass_z.filter(*cloud_filtered_z);

    return cloud_filtered_z;
}

myPointCloud::Ptr lidar_driver_info::pc_vox_filter(myPointCloud::Ptr cloud)
{
    lidar_params params;
    m_parent->get_lidar_params(params);
    auto leaf_size = params.voxel_leaf_size;
    pcl::VoxelGrid<myPoint> voxel_filter;
    voxel_filter.setInputCloud(cloud);
    voxel_filter.setLeafSize(leaf_size, leaf_size, leaf_size);
    myPointCloud::Ptr cloud_filtered(new myPointCloud);
    voxel_filter.filter(*cloud_filtered);
    return cloud_filtered;
}

void lidar_driver_info::tail_get_distance(myPointCloud::Ptr _cloud)
{
    auto pc_after_split_ret = split_cloud_to_side_and_content(_cloud, true);
    float max_x = std::numeric_limits<float>::lowest();
    for (const auto &itr : pc_after_split_ret.legal_side->points)
    {
        if (itr.x > max_x)
        {
            max_x = itr.x;
        }
    }
    update_distance(static_cast<double>(max_x));
    put_cloud(pc_after_split_ret.content);
    put_cloud(pc_after_split_ret.legal_side);
    put_cloud(pc_after_split_ret.illegal_side);
    update_cur_cloud();
}
static void color_cloud(int r, int g, int b, myPointCloud::Ptr _cloud)
{
    for (auto &itr : _cloud->points)
    {
        itr.r = r;
        itr.g = g;
        itr.b = b;
    }
}

pc_after_split lidar_driver_info::split_cloud_to_side_and_content(myPointCloud::Ptr _cloud, bool _x_plane)
{
    const std::string config_sec = "get_state";
    lidar_params params;
    m_parent->get_lidar_params(params);

    auto plane_DistanceThreshold = params.plane_distance_threshold;
    auto cluster_DistanceThreshold = params.cluster_distance_threshold;
    auto AngleThreshold = params.angle_threshold;
    auto cluster_require_points = params.cluster_required_point_num;

    auto start_us_stamp = get_current_us_stamp();
    pc_after_split ret;
    auto pickup_resp = pickup_pc_from_spec_range(_cloud);

    // 在范围内点云中拟合垂直y轴的平面,范围内点云染蓝色，平面染黄色
    pcl::ModelCoefficients::Ptr coe_side(new pcl::ModelCoefficients);
    Eigen::Vector3f plane_normal = _x_plane ? Eigen::Vector3f(1, 0, 0) : Eigen::Vector3f(0, 1, 0);
    auto fpop_ret = find_points_on_plane(pickup_resp->picked, plane_normal, plane_DistanceThreshold, AngleThreshold, coe_side, cluster_DistanceThreshold, cluster_require_points);
    auto clst_ret = find_max_cluster(fpop_ret.picked, cluster_DistanceThreshold, cluster_require_points);
    ret.content = pickup_resp->last;
    *ret.content += *fpop_ret.last;
    color_cloud(90, 23, 255, ret.content);
    ret.legal_side = clst_ret.picked;
    color_cloud(255, 255, 0, ret.legal_side);
    ret.illegal_side = clst_ret.last;

    auto end_us_stamp = get_current_us_stamp();
    return ret;
}

std::unique_ptr<pc_after_pickup> lidar_driver_info::pickup_pc_from_spec_range(myPointCloud::Ptr _orig_pc)
{
    const std::string config_sec = "get_state";
    lidar_params params;
    m_parent->get_lidar_params(params);

    auto x_min = params.second_range_x_min;
    auto x_max = params.second_range_x_max;
    auto y_min = params.second_range_y_min;
    auto y_max = params.second_range_y_max;
    auto result = std::make_unique<pc_after_pickup>();
    myPointCloud::Ptr tmp(new myPointCloud);

    split_cloud_by_pt(_orig_pc, "x", x_min, x_max, result->picked, tmp);
    *result->last += *tmp;
    split_cloud_by_pt(result->picked, "y", y_min, y_max, result->picked, tmp);
    *result->last += *tmp;

    return result;
}

void lidar_driver_info::split_cloud_by_pt(myPointCloud::Ptr _cloud, const std::string &_field, float _min, float _max, myPointCloud::Ptr &_cloud_filtered, myPointCloud::Ptr &_cloud_last)
{
    pcl::PassThrough<myPoint> pass;
    myPointCloud::Ptr tmp(new myPointCloud);
    pcl::copyPointCloud(*_cloud, *tmp);
    pass.setFilterFieldName(_field);
    pass.setFilterLimits(_min, _max);
    pass.setInputCloud(tmp);
    pass.filter(*_cloud_filtered);
    pass.setFilterLimitsNegative(true);
    pass.filter(*_cloud_last);
}

pc_after_pickup lidar_driver_info::find_points_on_plane(myPointCloud::Ptr _cloud, const Eigen::Vector3f &_ax_vec, float _distance_threshold, float _angle_threshold, pcl::ModelCoefficients::Ptr &_coe, float _cluster_distance_threshold, int _cluster_require_points)
{
    auto picked_up = myPointCloud::Ptr(new myPointCloud);
    pcl::PointIndices::Ptr one_plane(new pcl::PointIndices);
    pcl::SACSegmentation<myPoint> seg;
    seg.setModelType(pcl::SACMODEL_PERPENDICULAR_PLANE);
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setDistanceThreshold(_distance_threshold);
    seg.setOptimizeCoefficients(true);
    seg.setMaxIterations(1000);
    seg.setEpsAngle(pcl::deg2rad(_angle_threshold));
    seg.setAxis(_ax_vec);
    seg.setInputCloud(_cloud);
    seg.segment(*one_plane, *_coe);

    pcl::ExtractIndices<myPoint> extract;
    extract.setInputCloud(_cloud);
    extract.setIndices(one_plane);
    extract.setNegative(false);
    extract.filter(*picked_up);
    auto last = myPointCloud::Ptr(new myPointCloud);
    extract.setNegative(true);
    extract.filter(*last);

    pc_after_pickup ret;
    ret.picked = picked_up;
    ret.last = last;
    return ret;
}

pc_after_pickup lidar_driver_info::find_max_cluster(myPointCloud::Ptr _cloud, float _cluster_distance_threshold, int _cluster_require_points)
{
    pc_after_pickup ret;
    auto picked_up = _cloud;
    auto after_cluster = cluster_plane_points(picked_up, _cluster_distance_threshold, _cluster_require_points, 999999999);
    if (!after_cluster.empty())
    {
        picked_up = after_cluster[0];
        for (size_t i = 1; i < after_cluster.size(); ++i)
        {
            if (after_cluster[i]->points.size() > picked_up->points.size())
            {
                picked_up = after_cluster[i];
                *ret.last += *picked_up;
            }
            else
            {
                *ret.last += *(after_cluster[i]);
            }
        }
    }
    ret.picked = picked_up;

    return ret;
}

std::vector<myPointCloud::Ptr> lidar_driver_info::cluster_plane_points(myPointCloud::Ptr plane_points, float cluster_tolerance, int min_cluster_size, int max_cluster_size)
{
    std::vector<myPointCloud::Ptr> clusters;

    if (plane_points->empty())
    {
        return clusters;
    }

    // 创建KdTree用于搜索
    pcl::search::KdTree<myPoint>::Ptr tree(new pcl::search::KdTree<myPoint>);
    tree->setInputCloud(plane_points);

    // 欧几里得聚类
    std::vector<pcl::PointIndices> cluster_indices;
    pcl::EuclideanClusterExtraction<myPoint> ec;
    ec.setClusterTolerance(cluster_tolerance);
    ec.setMinClusterSize(min_cluster_size);
    ec.setMaxClusterSize(max_cluster_size);
    ec.setSearchMethod(tree);
    ec.setInputCloud(plane_points);
    ec.extract(cluster_indices);

    // 提取每个聚类
    for (const auto &indices : cluster_indices)
    {
        myPointCloud::Ptr cluster(new myPointCloud);
        pcl::copyPointCloud(*plane_points, indices, *cluster);
        clusters.push_back(cluster);
    }

    return clusters;
}

void lidar_driver_info::put_cloud(myPointCloud::Ptr _cloud)
{
    if (m_cur_cloud)
    {
        *m_cur_cloud += *_cloud;
    }
}

void lidar_driver_info::update_cur_cloud()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_output_cloud = m_cur_cloud;
    m_cur_cloud.reset(new myPointCloud);
}

void lidar_driver_info::head_get_distance(myPointCloud::Ptr _cloud)
{
    auto pc_after_split_ret = split_cloud_to_side_and_content(_cloud);
    lidar_params params;
    m_parent->get_lidar_params(params);
    auto line_DistanceThreshold = params.plane_distance_threshold;

    auto key_seg = get_key_seg(_cloud, line_DistanceThreshold, 0);
    m_side_z = key_seg.first.z;
    insert_several_points(_cloud, key_seg.first, key_seg.second);

    if (key_seg.second.x - key_seg.first.x > 2.3)
    {
        update_distance(static_cast<double>((key_seg.first.x)));
    }
    put_cloud(pc_after_split_ret.content);
    put_cloud(pc_after_split_ret.legal_side);
    put_cloud(pc_after_split_ret.illegal_side);
    update_cur_cloud();
}

myPointCloud::Ptr lidar_driver_info::get_output_cloud()
{
    myPointCloud::Ptr ret(new myPointCloud);
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_output_cloud)
    {
        pcl::copyPointCloud(*(m_output_cloud), *ret);
    }

    return ret;
}

void lidar_driver_info::log_cloud_size(myPointCloud::Ptr _cloud, const std::string &_tag)
{
    m_logger.log_print(al_log::LOG_LEVEL_INFO, "%s cloud size: %lu points", _tag.c_str(), _cloud->points.size());
}

std::unique_ptr<myPoint> lidar_driver_info::find_max_or_min_z_point(myPointCloud::Ptr _cloud, float _line_distance_threshold)
{
    auto ret = std::make_unique<myPoint>();
    ret->z = std::numeric_limits<float>::lowest();

    float x_min = std::numeric_limits<float>::max();
    float x_max = std::numeric_limits<float>::lowest();
    float y_bar = 0;
    // 遍历点云找到 最小x,最大x, 平均y
    for (const auto &point : _cloud->points)
    {
        if (point.x < x_min)
        {
            x_min = point.x;
        }
        if (point.x > x_max)
        {
            x_max = point.x;
        }
        y_bar += point.y;
    }
    ret->y = y_bar / _cloud->points.size();

    float all_z[30];
    for (int i = 0; i < sizeof(all_z) / sizeof(float); i++)
    {
        all_z[i] = std::numeric_limits<float>::lowest();
    }

    // 在x轴方向分30份，找出每一份紧凑点集中的最大z值
    for (int i = 0; i < sizeof(all_z) / sizeof(float); i++)
    {
        for (const auto &point : _cloud->points)
        {
            if (std::abs(point.x - (x_min + (x_max - x_min) * i / (sizeof(all_z) / sizeof(float)))) < _line_distance_threshold)
            {
                if (point.z > all_z[i])
                {
                    all_z[i] = point.z;
                }
            }
        }
    }

    // 计算得到一个平均最大z值的点
    float total_z = 0;
    for (int i = 0; i < sizeof(all_z) / sizeof(float); i++)
    {
        total_z += all_z[i];
    }
    ret->z = total_z / (sizeof(all_z) / sizeof(float));
    return ret;
}

float lidar_driver_info::pointToLineDistance(const Eigen::Vector3f &point, const Eigen::Vector3f &line_point, const Eigen::Vector3f &line_dir)
{
    Eigen::Vector3f diff = point - line_point;
    Eigen::Vector3f cross_product = diff.cross(line_dir);
    return cross_product.norm() / line_dir.norm();
}

void lidar_driver_info::insert_several_points(myPointCloud::Ptr _cloud, const myPoint &p1, const myPoint &p2, bool _is_red)
{
    _cloud->points.push_back(p1);
    for (int i = 1; i <= 100; ++i)
    {
        float t = static_cast<float>(i) / (100 + 1);
        myPoint p;
        p.x = p1.x + t * (p2.x - p1.x);
        p.y = p1.y + t * (p2.y - p1.y);
        p.z = p1.z + t * (p2.z - p1.z);
        p.r = 127;
        p.b = 255;
        p.g = 100;
        if (_is_red)
        {
            p.r = 255;
            p.b = 0;
            p.g = 0;
        }
        _cloud->points.push_back(p);
    }
    _cloud->points.push_back(p2);
}

void lidar_driver_info::serial_pc()
{
    m_update_counter++;
    if (m_update_counter >= 5)
    {
        m_update_counter = 0;
        serializePointCloud(get_output_cloud());
    }
}

std::pair<myPoint, myPoint> lidar_driver_info::get_key_seg(myPointCloud::Ptr _cloud, float line_distance_threshold, float _full_y_offset)
{
    auto max_z_point = find_max_or_min_z_point(_cloud, line_distance_threshold);

    max_z_point->x = 0;

    // 找出距离过上点平行于y轴的直线比较紧凑的若干点
    std::vector<myPoint> line_points;
    Eigen::Vector3f key_line_dir(1, 0, 0);
    for (auto &itr : _cloud->points)
    {
        auto tmp_point = itr;
        tmp_point.y = max_z_point->y;
        if (pointToLineDistance(tmp_point.getVector3fMap(), max_z_point->getVector3fMap(), key_line_dir) < line_distance_threshold)
        {
            line_points.push_back(itr);
        }
    }
    std::sort(
        line_points.begin(), line_points.end(),
        [](const myPoint &a, const myPoint &b)
        {
            return a.x < b.x;
        });
    if (line_points.empty())
    {
        return std::make_pair(*max_z_point, *max_z_point);
    }
    auto begin = line_points.front();
    auto end = line_points.front();
    float tmp_x = begin.x;

    // 找出刚才找到的点集中距离较大的两个点作为直线的两点并返回
    for (auto &itr : line_points)
    {
        if (itr.x - tmp_x < line_distance_threshold)
        {
            tmp_x = itr.x;
            end = itr;
        }
        else
        {
            break;
        }
    }
    begin.y = max_z_point->y + _full_y_offset;
    end.y = max_z_point->y + _full_y_offset;
    begin.z = max_z_point->z;
    end.z = max_z_point->z;

    return std::make_pair(begin, end);
}

void lidar_driver_info::update_distance(double _dist)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_distance = _dist;
}

void lidar_driver_info::start_driver(int _index)
{
    m_for_tail = (get_type_by_index(_index) == LIDAR_POS_TAIL);
    using namespace robosense::lidar;

    m_lidar_driver = new LidarDriver<pcMsg>();
    auto &driver = *(m_lidar_driver);
    m_lidar_index = _index;

    bind_callback2driver();
    driver.regExceptionCallback(
        [&](const Error &err)
        {
            m_logger.log_print(al_log::LOG_LEVEL_ERROR, "Lidar %d error msg=%s", m_lidar_index + 1, err.toString().c_str());
        });
    auto init_resp = driver.init(*init_rs_param(m_msop_port, m_difop_port));
    if (init_resp)
    {
        auto start_resp = driver.start();
        if (start_resp)
        {
            m_logger.log_print(al_log::LOG_LEVEL_INFO, "Lidar %d started successfully", m_lidar_index + 1);
        }
    }
}

double lidar_driver_info::get_distance()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_distance;
}
double lidar_driver_info::get_side_z()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_side_z;
}
static std::string make_file_name(const std::string &_filename = "")
{
    static int file_no = 0;
    auto file_name = "/database/file/" + _filename;
    if (_filename.empty())
    {
        file_name = "/tmp/ply/file_" + std::to_string(file_no++) + ".ply";
    }

    boost::filesystem::path dir = boost::filesystem::path(file_name).parent_path();
    if (!boost::filesystem::exists(dir))
    {
        boost::filesystem::create_directories(dir);
    }
    return file_name;
}
lidar_ply_info lidar_driver_info::save_ply2file(const std::string &_file_tag, bool _only_focus)
{
    lidar_ply_info ret;

    auto date_string = al_utils::ad_utils_date_time().m_datetime;
    date_string = al_utils::join_strings(al_utils::split_string(date_string, ' '), "_");

    auto focus_file = _file_tag + "_focus_" + date_string + ".ply";
    auto full_file = _file_tag + "_full_" + date_string + ".ply";

    auto file_name = make_file_name(focus_file);
    pcl::io::savePLYFileASCII(file_name, *get_output_cloud());
    if (!_only_focus)
    {
        file_name = make_file_name(full_file);
        pcl::io::savePLYFileASCII(file_name, *get_full_cloud());
    }

    ret.focus_ply_file = focus_file;
    ret.full_ply_file = full_file;

    return ret;
}

LIDAR_POS_TYPE lidar_driver_info::get_type_by_index(int _index)
{
    if (_index == 0 || _index == 1)
    {
        return LIDAR_POS_DROP;
    }
    else
    {
        return LIDAR_POS_TAIL;
    }
}

std::shared_ptr<pcMsg> lidar_driver_info::driverGetPointCloudFromCallerCallback()
{
    std::shared_ptr<pcMsg> msg = m_free_cloud_queue.pop();
    if (msg.get() != NULL)
    {
        return msg;
    }

    return std::make_shared<pcMsg>();
}

void lidar_driver_info::driverReturnPointCloudToCallerCallback(std::shared_ptr<pcMsg> msg)
{
    if (m_stuffed_cloud_queue.size() > 1)
    {
        m_logger.log_print(al_log::LOG_LEVEL_WARN, "Stuffed cloud queue is full, drop one cloud");
    }
    else
    {
        m_stuffed_cloud_queue.push(msg);
    }
}

void lidar_driver_info::processCloud()
{
    while (true)
    {
        std::shared_ptr<pcMsg> msg = m_stuffed_cloud_queue.popWait();
        if (msg.get() == NULL)
        {
            continue;
        }
        if (m_need_work)
        {
            auto begin_us_stamp = get_current_us_stamp();
            m_logger.log_print(al_log::LOG_LEVEL_DEBUG, "====start to process one cloud====");
            process_msg(msg);
            auto end_us_stamp = get_current_us_stamp();
            m_logger.log_print(al_log::LOG_LEVEL_DEBUG, "====process one cloud takes %ld us====", end_us_stamp - begin_us_stamp);
        }
        m_free_cloud_queue.push(msg);
    }
}

void lidar_driver_info::bind_callback2driver()
{
    m_lidar_driver->regPointCloudCallback(
        std::bind(&lidar_driver_info::driverGetPointCloudFromCallerCallback, this),
        std::bind(&lidar_driver_info::driverReturnPointCloudToCallerCallback, this, std::placeholders::_1));
    std::thread(
        [this]()
        {
            this->processCloud();
        })
        .detach();
}

void lidar_driver_info::process_msg(std::shared_ptr<pcMsg> msg)
{
    if (msg)
    {
        save_full_cloud(msg);
        auto one_frame = make_cloud_by_msg(msg);
        // 坐标转换
        auto frame_after_trans = pc_transform(one_frame);
        // 有效范围过滤
        auto frame_after_filter = pc_range_filter(frame_after_trans);
        log_cloud_size(frame_after_filter, "first filter");
        // 体素滤波
        frame_after_filter = pc_vox_filter(frame_after_filter);
        log_cloud_size(frame_after_filter, "voxel filter");
        if (m_for_tail)
        {
            tail_get_distance(frame_after_filter);
        }
        else
        {
            head_get_distance(frame_after_filter);
        }
    }
}

myPointCloud::Ptr lidar_driver_info::make_cloud_by_msg(std::shared_ptr<pcMsg> _msg)
{
    lidar_params params;
    m_parent->get_lidar_params(params);
    myPointCloud::Ptr one_frame(new myPointCloud);
    for (size_t i = 0; i < _msg->points.size(); i++)
    {
        if (_msg->points[i].intensity < params.first_range_i_min || _msg->points[i].intensity > params.first_range_i_max)
        {
            continue;
        }
        myPoint tmp;
        tmp.x = _msg->points[i].x;
        tmp.y = _msg->points[i].y;
        tmp.z = _msg->points[i].z;
        tmp.r = 255;
        tmp.g = 255;
        tmp.b = 255;
        one_frame->push_back(tmp);
    }

    return one_frame;
}
