#include "../lib/pid_control_lib.h"
#include "../../public/lib/ad_rpc.h"

#include <iostream>
#include <random>
#include <chrono>

class ItObject{
    double m_self = -2.8;
    double m_delta_time = 0.2 * 0.08;
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

class ControlledSystem {
private:
    double output;      // 当前输出
    double k;           // 比例系数
    double dt;          // 时间步长（用于数值积分）

public:
    // 构造函数
    ControlledSystem(double initialOutput = 1.0, double gain = 1.0, double timeStep = 0.01)
        : output(initialOutput), k(gain), dt(timeStep) {
        if (initialOutput <= 0) {
            output = 0.0001;  // 避免初始值为0时系统不工作
        }
    }

    // 设置输入并更新输出
    void setInput(int u) {
        // 只接受-1, 0, 1三种输入
        if (u != -1 && u != 0 && u != 1) {
            return;
        }

        // 使用欧拉法数值积分
        double derivative = k * output * u;
        output += derivative * dt;

        // 防止输出变为0或负数（根据实际需求调整）
        if (output < 1e-10) {
            output = 1e-10;
        }
    }

    // 获取当前输出
    double getOutput() const {
        return output;
    }

    // 设置参数
    void setParameters(double gain, double timeStep) {
        k = gain;
        dt = timeStep;
    }

    // 重置系统
    void reset(double initialOutput = 1.0) {
        output = initialOutput;
        if (output <= 0) {
            output = 1.0;
        }
    }

    // 析构函数
    ~ControlledSystem() = default;
};

#include <cmath>
#include <stdexcept>

class ControlledObject {
private:
    double output;   // 当前输出值
    double k1;        // 比例系数，控制变化速度
    double k2;
    double dt;       // 时间步长（离散模拟的时间间隔）
    const double EPS = 1e-9;  // 防止除零和过小的阈值

public:
    // 构造函数：初始化输出值、比例系数和时间步长
    ControlledObject(double initialOutput = 1.0, double coefficient = 1.0, double timeStep = 0.01)
        : k1(coefficient), k2(4), dt(timeStep) {
        if (initialOutput <= 0) {
            throw std::invalid_argument("Initial output must be positive.");
        }
        if (coefficient <= 0 || timeStep <= 0) {
            throw std::invalid_argument("Coefficient and time step must be positive.");
        }
        output = initialOutput;
    }

    // 根据输入更新输出值
    void update(int input) {
        if (input == 0) {
            // 输入为0，输出保持不动
            return;
        } else if (input == 1) {
            // 输入为1，输出变大，变化速度与当前输出成正比
            output += k1 * output * dt;
        } else if (input == -1) {
            // 输入为-1，输出变小，变化速度与当前输出成反比
            // 当输出非常小时，避免除零和过大变化
            if (output > EPS) {
                output -= k2 / output * dt;
                // 防止输出变为负数或过小
                if (output < EPS) {
                    output = EPS;
                }
            }
        } else {
            throw std::invalid_argument("Input must be -1, 0, or 1.");
        }
    }

    // 获取当前输出值
    double getOutput() const {
        return output;
    }

    // 设置时间步长
    void setTimeStep(double timeStep) {
        if (timeStep <= 0) {
            throw std::invalid_argument("Time step must be positive.");
        }
        dt = timeStep;
    }
};
class LaggingPlant {
private:
    double output;              // 当前输出值
    double time;               // 当前内部时间
    double dt;                 // 时间步长
    double gain;               // 增益系数
    double max_lag;            // 最大滞后时间
    double min_lag;            // 最小滞后时间
    double lag_sensitivity;    // 滞后对输出的敏感度

    // 滞后队列：存储(时间, 输入)对
    std::deque<std::pair<double, double>> lag_queue;

    // 根据当前输出计算滞后时间
    double calculateLagTime() const {
        // 输出越大，滞后越小；输出越小，滞后越大
        // 使用sigmoid函数使滞后在min_lag和max_lag之间平滑变化
        double normalized_output = output / 10.0; // 假设输出范围大致在[-10, 10]
        double sigmoid = 1.0 / (1.0 + std::exp(lag_sensitivity * normalized_output));

        // 将sigmoid值映射到[min_lag, max_lag]范围
        return min_lag + (max_lag - min_lag) * sigmoid;
    }

    // 从滞后队列获取有效输入
    double getEffectiveInput(double current_time) {
        double lag_time = calculateLagTime();
        double target_time = current_time - lag_time;

        // 如果队列为空或目标时间早于最早记录，返回0
        if (lag_queue.empty() || target_time <= lag_queue.front().first) {
            return 0.0;
        }

        // 查找最接近目标时间的输入
        auto it = lag_queue.begin();
        auto prev_it = it;

        for (; it != lag_queue.end(); ++it) {
            if (it->first > target_time) {
                break;
            }
            prev_it = it;
        }

        // 返回目标时间点的有效输入
        return prev_it->second;
    }

public:
    // 构造函数
    LaggingPlant(double initial_output = 0.0,
                 double time_step = 0.1,
                 double gain_value = 0.5,
                 double min_lag_time = 0.5,
                 double max_lag_time = 2.0,
                 double sensitivity = 2.0)
        : output(initial_output)
        , time(0.0)
        , dt(time_step)
        , gain(gain_value)
        , min_lag(min_lag_time)
        , max_lag(max_lag_time)
        , lag_sensitivity(sensitivity) {

        // 初始化滞后队列
        lag_queue.push_back(std::make_pair(time, 0.0));
    }

    // 设置输入：1增加，0保持，-1减少
    void setInput(int input) {
        // 记录当前输入到滞后队列
        lag_queue.push_back(std::make_pair(time, static_cast<double>(input)));

        // 清理过期数据（超过最大滞后时间2倍的数据）
        double expire_time = time - 2.0 * max_lag;
        while (!lag_queue.empty() && lag_queue.front().first < expire_time) {
            lag_queue.pop_front();
        }
    }

    // 更新被控对象状态
    void update() {
        // 获取有效输入（考虑滞后）
        double effective_input = getEffectiveInput(time);

        // 根据有效输入更新输出
        if (effective_input > 0.5) {        // 输入为1
            output += gain * dt;
        } else if (effective_input < -0.5) { // 输入为-1
            output -= gain * dt;
        }
        // 输入为0时，输出保持不变

        // 更新时间
        time += dt;

        // 输出饱和限制（可选）
        const double output_limit = 20.0;
        if (output > output_limit)
            output = output_limit;
        if (output < -output_limit)
            output = -output_limit;
        if (output < 0)
        {
            output = 0;
        }
    }

    // 获取当前输出
    double getOutput() const
    {
        return output;
    }

    // 获取当前滞后时间
    double getCurrentLagTime() const
    {
        return calculateLagTime();
    }

    // 获取当前时间
    double getCurrentTime() const
    {
        return time;
    }

    // 打印状态信息
    void printStatus() const
    {
        std::cout << "Time: " << time
                  << ", Output: " << output
                  << ", Lag Time: " << calculateLagTime()
                  << std::endl;
    }
};

int main(int argc, char const *argv[])
{
    auto Skp = atof(argv[1]);
    auto Ski = atof(argv[2]);
    auto Skd = atof(argv[3]);
    auto SDZ = atof(argv[4]);
    auto Ckp = atof(argv[5]);
    auto Cki = atof(argv[6]);
    auto Ckd = atof(argv[7]);
    auto LGg = atof(argv[8]);
    auto LGc = atof(argv[9]);
    auto MaxSpeed = atof(argv[10]);
    auto smith_k = atof(argv[11]);
    auto smith_T = atof(argv[12]);
    auto smith_tau = atof(argv[13]);
    LaggingPlant co(0, 0.08, LGg, 0.5, 3.5, LGc);
    ItObject io(0.04);
    pid_control::DiscretePID pc_inner(Skp, Ski, Skd, SDZ, 10, 0.08);
    pid_control::DiscretePID pc_outer(Ckp, Cki, Ckd, 0, 10, 0.08);
    pid_control::SmithPredictor smith(smith_k, smith_T, smith_tau, 0.08);

    std::vector<double> expect_array;
    for (int i = 0; i < 200; i++)
    {
        expect_array.push_back(-0.6);
    }
    for (auto spec : expect_array)
    {
        auto expect_output = spec;
        auto outer_output = pc_outer.execute_continuous(io.getOutput(), expect_output);
        if (outer_output > MaxSpeed)
        {
            outer_output = MaxSpeed;
        }
        auto smith_output = smith.estimate(co.getOutput());
        auto control_signal = pc_inner.execute(smith_output, outer_output);
        smith.updateControl(control_signal);
        co.setInput(control_signal);
        co.update();

        auto co_output = co.getOutput();
        auto output = io.update(co_output);
        std::cout << expect_output << "," << outer_output << "," << control_signal << "," << co_output << "," << output << std::endl;
    }
    return 0;
}
