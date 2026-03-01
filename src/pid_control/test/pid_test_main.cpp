#include "../lib/pid_control_lib.h"
#include "../../public/lib/ad_rpc.h"

#include <iostream>
#include <random>
#include <chrono>

class ControlledObject
{
private:
    double output_;           // 当前输出值
    double min_output_;       // 最小输出值
    double max_output_;       // 最大输出值
    double step_size_;        // 每次变化的步长
    double decay_rate_;       // 衰减率，当输入为0时，输出减少的速率
    double noise_level_;      // 噪声水平
    double disturbance_;      // 外部扰动
    bool auto_decay_enabled_; // 是否启用自动衰减

    // 随机数生成器，用于添加噪声
    std::mt19937 rng_;
    std::normal_distribution<double> normal_dist_;

public:
    /**
     * @brief 构造函数
     * @param initial_output 初始输出值
     * @param min_output 最小输出值，默认0
     * @param max_output 最大输出值，默认1000
     * @param step_size 每次变化的步长，默认1.0
     * @param decay_rate 衰减率，当输入为0时输出减少的速率，默认0.5
     * @param noise_level 噪声水平，默认0.0
     * @param disturbance 固定扰动，默认0.0
     */
    ControlledObject(double initial_output = 0.0,
                     double min_output = 0.0,
                     double max_output = 1000.0,
                     double step_size = 1.0,
                     double decay_rate = 0.5,
                     double noise_level = 0.0,
                     double disturbance = 0.0)
        : output_(initial_output),
          min_output_(min_output),
          max_output_(max_output),
          step_size_(step_size),
          decay_rate_(decay_rate),
          noise_level_(noise_level),
          disturbance_(disturbance),
          auto_decay_enabled_(true)
    { // 默认启用自动衰减

        // 初始化随机数生成器
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        rng_.seed(seed);
        normal_dist_ = std::normal_distribution<double>(0.0, 1.0);

        // 验证参数
        validateParameters();
    }

    /**
     * @brief 接收控制信号并更新输出
     * @param control_signal 控制信号：1(增大), 0(缓慢减少), -1(减小)
     * @return 更新后的输出值
     */
    double update(int control_signal)
    {
        // 根据控制信号更新输出
        switch (control_signal)
        {
        case 1: // 增大
            output_ += step_size_;
            break;
        case -1: // 减小
            output_ -= step_size_;
            break;
        case 0: // 缓慢减少
            if (auto_decay_enabled_ && output_ > min_output_)
            {
                output_ -= decay_rate_;
                // 确保不会降到最小值以下
                if (output_ < min_output_)
                {
                    output_ = min_output_;
                }
            }
            // 如果不启用自动衰减，则保持当前值
            break;
        default:
            // 无效信号，保持当前值
            break;
        }

        // 添加外部扰动
        output_ += disturbance_;

        // 添加随机噪声
        if (noise_level_ > 0.0)
        {
            output_ += normal_dist_(rng_) * noise_level_;
        }

        // 限制输出在[min_output_, max_output_]范围内
        if (output_ > max_output_)
        {
            output_ = max_output_;
        }
        else if (output_ < min_output_)
        {
            output_ = min_output_;
        }

        return output_;
    }

    /**
     * @brief 直接设置输出值
     * @param value 要设置的值
     * @return 设置后的输出值
     */
    double setOutput(double value)
    {
        output_ = value;
        // 确保在范围内
        if (output_ > max_output_)
        {
            output_ = max_output_;
        }
        else if (output_ < min_output_)
        {
            output_ = min_output_;
        }
        return output_;
    }

    /**
     * @brief 获取当前输出值
     */
    double getOutput() const
    {
        return output_;
    }

    /**
     * @brief 获取最小输出值
     */
    double getMinOutput() const
    {
        return min_output_;
    }

    /**
     * @brief 获取最大输出值
     */
    double getMaxOutput() const
    {
        return max_output_;
    }

    /**
     * @brief 获取衰减率
     */
    double getDecayRate() const
    {
        return decay_rate_;
    }

    /**
     * @brief 设置步长
     */
    void setStepSize(double step_size)
    {
        if (step_size >= 0)
        {
            step_size_ = step_size;
        }
    }

    /**
     * @brief 设置衰减率
     */
    void setDecayRate(double decay_rate)
    {
        if (decay_rate >= 0)
        {
            decay_rate_ = decay_rate;
        }
    }

    /**
     * @brief 启用或禁用自动衰减
     */
    void setAutoDecayEnabled(bool enabled)
    {
        auto_decay_enabled_ = enabled;
    }

    /**
     * @brief 检查自动衰减是否启用
     */
    bool isAutoDecayEnabled() const
    {
        return auto_decay_enabled_;
    }

    /**
     * @brief 设置噪声水平
     */
    void setNoiseLevel(double noise_level)
    {
        if (noise_level >= 0)
        {
            noise_level_ = noise_level;
        }
    }

    /**
     * @brief 设置外部扰动
     */
    void setDisturbance(double disturbance)
    {
        disturbance_ = disturbance;
    }

    /**
     * @brief 重置对象到初始状态
     * @param initial_output 初始输出值
     */
    void reset(double initial_output = 0.0)
    {
        output_ = initial_output;
        validateParameters();
    }

    /**
     * @brief 模拟系统固有延迟
     * @param delay_ms 延迟时间（毫秒）
     */
    void simulateDelay(int delay_ms) const
    {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start)
                   .count() < delay_ms)
        {
            // 空循环，模拟延迟
        }
    }

    /**
     * @brief 模拟非线性的被控对象
     * @param control_signal 控制信号
     * @param nonlinear_factor 非线性因子，0.0表示线性，>0.0表示非线性
     * @return 更新后的输出值
     */
    double updateWithNonlinearity(int control_signal, double nonlinear_factor = 0.1)
    {
        double effective_step = step_size_;
        double effective_decay = decay_rate_;

        // 根据当前位置添加非线性
        if (output_ > max_output_ * 0.8)
        {
            // 接近上限时，增加阻力
            effective_step *= (1.0 - nonlinear_factor);
            effective_decay *= (1.0 + nonlinear_factor); // 衰减更快
        }
        else if (output_ < min_output_ + (max_output_ - min_output_) * 0.2)
        {
            // 接近下限时，增加阻力
            effective_step *= (1.0 - nonlinear_factor);
            effective_decay *= (1.0 - nonlinear_factor * 0.5); // 衰减更慢
        }

        // 保存原始参数
        double original_step = step_size_;
        double original_decay = decay_rate_;

        // 临时修改参数
        step_size_ = effective_step;
        decay_rate_ = effective_decay;

        // 更新输出
        double result = update(control_signal);

        // 恢复原始参数
        step_size_ = original_step;
        decay_rate_ = original_decay;

        return result;
    }

    /**
     * @brief 指数衰减模式
     * @param control_signal 控制信号
     * @return 更新后的输出值
     *
     * 当控制信号为0时，输出按指数衰减：output = output * decay_factor
     * 其中 decay_factor 是 (1 - decay_rate_/100)
     */
    double updateWithExponentialDecay(int control_signal)
    {
        if (control_signal == 0 && auto_decay_enabled_ && output_ > min_output_)
        {
            // 指数衰减
            double decay_factor = 1.0 - decay_rate_ / 100.0;
            if (decay_factor < 0)
                decay_factor = 0;

            output_ *= decay_factor;

            // 如果值非常小，直接设为0
            if (output_ < 0.01)
            {
                output_ = min_output_;
            }
        }
        else
        {
            // 正常更新
            return update(control_signal);
        }

        // 添加外部扰动
        output_ += disturbance_;

        // 添加随机噪声
        if (noise_level_ > 0.0)
        {
            output_ += normal_dist_(rng_) * noise_level_;
        }

        // 确保在范围内
        if (output_ > max_output_)
        {
            output_ = max_output_;
        }
        else if (output_ < min_output_)
        {
            output_ = min_output_;
        }

        return output_;
    }

private:
    /**
     * @brief 验证和修正参数
     */
    void validateParameters()
    {
        // 确保最小输出不大于最大输出
        if (min_output_ > max_output_)
        {
            std::swap(min_output_, max_output_);
        }

        // 确保初始输出在范围内
        if (output_ < min_output_)
        {
            output_ = min_output_;
        }
        else if (output_ > max_output_)
        {
            output_ = max_output_;
        }

        // 确保步长非负
        if (step_size_ < 0)
        {
            step_size_ = 1.0;
        }

        // 确保衰减率非负
        if (decay_rate_ < 0)
        {
            decay_rate_ = 0.5;
        }

        // 确保噪声水平非负
        if (noise_level_ < 0)
        {
            noise_level_ = 0.0;
        }
    }
};

class ItObject{
    double m_self = -2.8;
    double m_delta_time = 0.1;
public:
    ItObject(double _delta_time):m_delta_time(_delta_time){}
    double update(double _input){
        auto ret = m_self + _input * m_delta_time;
        m_self = ret;
        return ret;
    }
    double getOutput(){
        return m_self;
    }
};

int main(int argc, char const *argv[])
{
    auto Skp = atof(argv[1]);
    auto Ski = atof(argv[2]);
    auto Skd = atof(argv[3]);
    auto Ckp = atof(argv[4]);
    ControlledObject co(0, 0, 3, 0.02, 0.002, 0.0005, 0.0003);
    ItObject io(0.33);
    pid_control::DiscretePID pc_inner(Skp, Ski, Skd, 0.008, 10, 0.33);
    pid_control::DiscretePID pc_outer(Ckp, 0, 0, 0, 10, 0.33);

    std::vector<double> expect_array;
    for (int i = 0; i < 200; i++)
    {
        expect_array.push_back(-0.6);
    }
    for (auto spec : expect_array)
    {
        auto expect_output = spec;
        auto outer_output = pc_outer.execute_continuous(io.getOutput(), expect_output);
        auto control_signal = pc_inner.execute(co.getOutput(), outer_output);
        auto co_output = co.update(control_signal);
        auto output = io.update(co_output);
        std::cout << expect_output <<","<<outer_output << ","<<control_signal<<","<<  co_output <<","<< output << std::endl;
    }
    return 0;
}
