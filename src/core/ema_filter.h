#pragma once

// 指数移动平均滤波器 - 平滑数据波动
class EMAFilter {
private:
    double alpha;       // 平滑系数 (0-1)
    double value;       // 当前值
    bool initialized;   // 是否已初始化
    
public:
    EMAFilter(double smoothFactor = 0.3) 
        : alpha(smoothFactor), value(0), initialized(false) {
        // alpha越小越平滑，但响应越慢
        // 建议值：0.1-0.3
    }
    
    double Update(double newValue) {
        if (!initialized) {
            value = newValue;
            initialized = true;
        } else {
            value = alpha * newValue + (1.0 - alpha) * value;
        }
        return value;
    }
    
    double GetValue() const {
        return value;
    }
    
    void Reset() {
        value = 0;
        initialized = false;
    }
    
    void SetSmoothFactor(double factor) {
        if (factor >= 0 && factor <= 1) {
            alpha = factor;
        }
    }
};