#if !defined(_PID_CONTROL_LIB_H_)
#define _PID_CONTROL_LIB_H_

#include <iostream>
#include <cmath>
#include <chrono>
#include <queue>

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
    class FixedWindowRateCalculator
    {
    private:
        struct DataPoint
        {
            double value;
            std::chrono::steady_clock::time_point time;
        };

        std::queue<DataPoint> data_queue_;
        const uint32_t window_size_;
        uint32_t count_;
    public:
        // 构造函数，指定窗口大小
        FixedWindowRateCalculator(uint32_t window_size = 10);
        // 周期性输入数值
        void input(double value);
        // 获取速率
        double get_rate();
        // 获取窗口内数据点数量
        uint32_t get_point_count() const
        {
            return count_;
        }
        // 重置计数器
        void reset()
        {
            std::queue<DataPoint> empty;
            std::swap(data_queue_, empty);
            count_ = 0;
        }
    };
/**
 * @class SmithPredictor
 * @brief 史密斯预估器实现，用于补偿一阶惯性加纯滞后对象
 *
 * 原理：内部建立一个与真实对象（不含滞后环节）相同的模型，
 * 用其输出与真实对象的滞后输出进行比较，补偿滞后效应。
 */
class SmithPredictor {
private:
    // 模型参数
    double K_;         // 对象增益
    double T_;         // 对象时间常数
    double tau_;       // 纯滞后时间 (θ)
    double Ts_;        // 采样周期

    // 滞后步数（离散化）
    int d_;  // 滞后步数 = round(tau_ / Ts_)

    // 内部模型离散化参数（一阶后向差分）
    double a_;     // a = T_ / Ts_
    double alpha_; // alpha = a_ / (a_ + 1)
    double beta_;  // beta = K_ / (a_ + 1)

    // 内部状态
    double y_hat_;      // 内部模型的无滞后输出 y_hat(k)
    double u_prev_;     // 上一时刻的控制量 u(k-1)

    // 循环缓冲区，用于存储滞后输出
    std::vector<double> delay_buffer_;  // 存储 y_hat 的历史值
    int buffer_index_;                  // 缓冲区当前索引

public:
    /**
     * @brief 构造函数
     * @param K 对象增益
     * @param T 对象时间常数
     * @param tau 纯滞后时间
     * @param Ts 采样周期
     */
    SmithPredictor(double K, double T, double tau, double Ts) ;


    /**
     * @brief 核心计算函数：根据当前测量值y，计算并返回补偿后的反馈信号y_p
     * @param y 当前实际对象输出 y(k)
     * @return 补偿后的反馈信号 y_p(k)（用于PID控制器的输入）
     *
     * 控制循环调用顺序：
     * 1. y_p = sp.estimate(y_measured);
     * 2. e = r - y_p;
     * 3. u = pid.calculate(e);
     * 4. sp.updateControl(u);
     * 5. 输出u到被控对象
     */
    double estimate(double y);
    /**
     * @brief 更新控制量（在PID计算后调用）
     * @param u 当前时刻计算出的控制量 u(k)
     */
    void updateControl(double u) {
        u_prev_ = u;  // 保存当前控制量，供下一步使用
    }

    // 辅助函数：重置预估器状态
    void reset() {
        y_hat_ = 0.0;
        u_prev_ = 0.0;
        std::fill(delay_buffer_.begin(), delay_buffer_.end(), 0.0);
        buffer_index_ = 0;
    }

    // 获取滞后步数
    int getDelaySteps() const { return d_; }

    // 获取内部模型输出（调试用）
    double getInternalOutput() const { return y_hat_; }
};
} // namespace pid_control

#endif // _PID_CONTROL_LIB_H_
