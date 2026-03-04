#include "pid_control_lib.h"

pid_control::DiscretePID::DiscretePID(
    double kp,
    double ki,
    double kd,
    double deadband,
    double output_limit,
    double delta_time) : kp_(kp), ki_(ki), kd_(kd),
                        deadband_(deadband),
                         integral_(0.0), prev_error_(0.0),
                         output_limit_(output_limit),
                         delta_time_(delta_time), first_run_(true)
{
    if (deadband_ < 0)
    {
        deadband_ = 0;
    }

    if (delta_time_ <= 0)
    {
        delta_time_ = 0.01;
    }
}

int pid_control::DiscretePID::execute(double measured_value, double setpoint)
{
    // 计算误差
    double error = setpoint - measured_value;

    // 检查死区
    if (std::abs(error) <= deadband_)
    {
        reset_integral();
        prev_error_ = error;
        return 0;
    }

    // 如果是第一次运行，初始化状态
    if (first_run_)
    {
        prev_error_ = error;
        first_run_ = false;
        return calculate_output(0, error);
    }

    // 计算积分项（考虑积分限制）
    integral_ += error * delta_time_;

    // 限制积分项防止积分饱和
    if (integral_ > output_limit_)
    {
        integral_ = output_limit_;
    }
    else if (integral_ < -output_limit_)
    {
        integral_ = -output_limit_;
    }

    // 计算微分项
    double derivative = (error - prev_error_) / delta_time_;

    // 计算PID输出
    double output = kp_ * error + ki_ * integral_ + kd_ * derivative;

    // 更新上一次误差
    prev_error_ = error;

    // 计算离散输出
    return calculate_output(output, error);
}

double pid_control::DiscretePID::execute_continuous(double measured_value, double setpoint)
{
    // 计算误差
    double error = setpoint - measured_value;

    // 如果是第一次运行，初始化状态
    if (first_run_)
    {
        prev_error_ = error;
        first_run_ = false;
    }

    // 计算积分项（考虑积分限制）
    integral_ += error * delta_time_;

    // 限制积分项防止积分饱和
    if (integral_ > output_limit_)
    {
        integral_ = output_limit_;
    }
    else if (integral_ < -output_limit_)
    {
        integral_ = -output_limit_;
    }

    // 计算微分项
    double derivative = (error - prev_error_) / delta_time_;

    // 计算PID输出
    double output = kp_ * error + ki_ * integral_ + kd_ * derivative;

    // 更新上一次误差
    prev_error_ = error;
    return output;
}

int pid_control::DiscretePID::calculate_output(double pid_output, double error)
{
    // 如果PID输出超过阈值，则输出相应方向
    if (pid_output > 0)
    {
        return 1; // 增大
    }
    else if (pid_output < 0)
    {
        return -1; // 减小
    }

    return 0; // 默认保持
}

pid_control::FixedWindowRateCalculator::FixedWindowRateCalculator(uint32_t window_size) : window_size_(window_size), count_(0)
{
    if (window_size_ < 2)
    {
        throw std::invalid_argument("Window size must be at least 2");
    }
}

void pid_control::FixedWindowRateCalculator::input(double value)
{
    DataPoint dp;
    dp.value = value;
    dp.time = std::chrono::steady_clock::now();

    data_queue_.push(dp);
    count_++;

    // 保持队列大小不超过窗口大小
    if (count_ > window_size_)
    {
        data_queue_.pop();
        count_--;
    }
}

double pid_control::FixedWindowRateCalculator::get_rate()
{
    if (count_ < 2)
    {
        return 0.0; // 不足两个点，无法计算速率
    }

    auto &newest = data_queue_.back();
    auto &oldest = data_queue_.front();

    // 计算时间间隔
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(
        newest.time - oldest.time);

    if (duration.count() <= 0)
    {
        return 0.0;
    }

    // 计算速率
    double value_diff = newest.value - oldest.value;
    return value_diff / duration.count();
}

pid_control::SmithPredictor::SmithPredictor(double K, double T, double tau, double Ts)
    : K_(K), T_(T), tau_(tau), Ts_(Ts)
{
    // 1. 计算滞后步数
    d_ = static_cast<int>(std::round(tau_ / Ts_));
    if (d_ < 0)
        d_ = 0;

    // 2. 计算一阶惯性环节离散化参数（后向差分法）
    a_ = T_ / Ts_;
    alpha_ = a_ / (a_ + 1.0);
    beta_ = K_ / (a_ + 1.0);

    // 3. 初始化内部状态
    y_hat_ = 0.0;
    u_prev_ = 0.0;

    // 4. 初始化滞后缓冲区（长度为d_+1，用0填充）
    delay_buffer_.resize(d_ + 1, 0.0);
    buffer_index_ = 0;
}

double pid_control::SmithPredictor::estimate(double y)
{
    // 1. 计算内部模型当前输出 y_hat(k) （使用上一时刻的控制量u_prev_）
    double y_hat_k = alpha_ * y_hat_ + beta_ * u_prev_;

    // 2. 从缓冲区获取滞后d步的内部模型输出 y_hat(k-d)
    int delay_index = (buffer_index_ - d_ + (d_ + 1)) % (d_ + 1);
    double y_hat_k_d = delay_buffer_[delay_index];

    // 3. 计算史密斯补偿误差
    double e_smith = y - y_hat_k_d;

    // 4. 计算补偿后的反馈信号
    double y_p = y_hat_k + e_smith;

    // 5. 更新内部状态（为下一步做准备）
    y_hat_ = y_hat_k; // 更新内部模型输出

    // 将当前y_hat_k存入缓冲区
    delay_buffer_[buffer_index_] = y_hat_k;
    buffer_index_ = (buffer_index_ + 1) % (d_ + 1);

    return y_p;
}
