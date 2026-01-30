#if !defined(_PID_CONTROL_LIB_H_)
#define _PID_CONTROL_LIB_H_

#include <iostream>
#include <cmath>
#include <chrono>

namespace pid_control
{
    class DiscretePID
    {
    private:
        // PID参数
        double kp_; // 比例系数
        double ki_; // 积分系数
        double kd_; // 微分系数

        // 控制参数
        double deadband_;  // 死区范围，误差在此范围内输出0

        // 内部状态
        double integral_;     // 积分项累积
        double prev_error_;   // 上一次误差
        double output_limit_; // 输出限制

        // 时间相关
        std::chrono::steady_clock::time_point prev_time_;
        double delta_time_; // 采样周期（秒）
        bool first_run_;    // 首次运行标志

    public:
        /**
         * @brief 构造函数
         * @param kp 比例系数
         * @param ki 积分系数
         * @param kd 微分系数
         * @param deadband 死区范围，默认0.1
         * @param output_limit 积分项限制，默认10.0
         * @param delta_time 采样周期（秒），默认0.01
         */
        DiscretePID(double kp, double ki, double kd,
                    double deadband = 0.1,
                    double output_limit = 10.0,
                    double delta_time = 0.01);


        /**
         * @brief 执行PID控制
         * @param measured_value 测量值
         * @param setpoint 设定值
         * @return 离散输出：1(增大), -1(减小), 0(保持)
         */
        int execute(double measured_value, double setpoint);
        double execute_continuous(double measured_value, double setpoint);


        /**
         * @brief 重置控制器状态
         */
        void reset()
        {
            integral_ = 0.0;
            prev_error_ = 0.0;
            first_run_ = true;
        }

        /**
         * @brief 设置PID参数
         */
        void setParameters(double kp, double ki, double kd)
        {
            kp_ = kp;
            ki_ = ki;
            kd_ = kd;
        }

        /**
         * @brief 设置死区和阈值
         */
        void setLimits(double deadband)
        {
            deadband_ = (deadband >= 0) ? deadband : 0;
        }

        /**
         * @brief 设置采样周期
         */
        void setDeltaTime(double delta_time)
        {
            if (delta_time > 0)
            {
                delta_time_ = delta_time;
            }
        }

        /**
         * @brief 获取当前积分值
         */
        double getIntegral() const
        {
            return integral_;
        }

        /**
         * @brief 获取上一次误差
         */
        double getPreviousError() const
        {
            return prev_error_;
        }

    private:
        /**
         * @brief 根据PID输出和误差计算离散输出
         */
        int calculate_output(double pid_output, double error);


        /**
         * @brief 重置积分项
         */
        void reset_integral()
        {
            integral_ = 0.0;
        }
    };
} // namespace pid_control

#endif // _PID_CONTROL_LIB_H_
