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
