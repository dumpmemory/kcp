//=====================================================================
//
// AbstractStats.h - 
//
// Last Modified: 2020/04/03 17:47:56
//
//=====================================================================
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <map>
#include <deque>

#ifndef NAMESPACE_BEGIN
#define NAMESPACE_BEGIN(x) namespace x {
#endif

#ifndef NAMESPACE_END
#define NAMESPACE_END(x) }
#endif


NAMESPACE_BEGIN(System);

//---------------------------------------------------------------------
// unsined integer unwrapper
//---------------------------------------------------------------------
class UnsignedWrap
{
public:
	UnsignedWrap();
	UnsignedWrap(int size);
	UnsignedWrap(const UnsignedWrap& src);

	UnsignedWrap& operator = (const UnsignedWrap& src);

public:
	int64_t wrap_uint16(uint16_t val);
	int64_t wrap_uint32(uint32_t val);

	void reset(int size);
	void set_last(int64_t last);

protected:
	static bool u16_is_newer(uint16_t val, uint16_t prev_val);
	static bool u32_is_newer(uint32_t val, uint32_t prev_val);

	int64_t u16_update(uint16_t val);
	int64_t u32_update(uint32_t val);

protected:
	int size;
	int64_t last_value;
};


//---------------------------------------------------------------------
// 带时间窗口的丢包统计
//---------------------------------------------------------------------
class AbstractLossStats
{
public:
	virtual ~AbstractLossStats();
	AbstractLossStats();
	AbstractLossStats(const AbstractLossStats& src);
	AbstractLossStats(AbstractLossStats&& src);

	AbstractLossStats& operator = (const AbstractLossStats& src);

public:

	// 初始化，默认统计 4 秒窗口，最大包数为 200，至少 100 个包开始统计
	void init(int window_ms = 4000, int window_num = 200, int limit = 100);

	// 复位
	void reset();

	// 收到一个包时调用
	void update(uint16_t seq, int64_t now_ts);	

	// 计算丢包率：fraction_loss 是返回的小数丢包率，255 代表 100%
	// 成功返回 0，包不够统计就返回 -1
	int calculate(int64_t now_ts, uint8_t *fraction_loss, int *num);

protected:

	// 淘汰超过窗口的太老的数据
	void evict_oldest(int64_t now_ts);

protected:

	// 使用有序字典保存收包信息，key=id, value=ts, 越小的 id 越在前面
	typedef std::map<int64_t, int64_t, std::less<int64_t>> LossWindow;

	LossWindow _loss_window;          // 保存一段时间窗口的数据
	UnsignedWrap _wrapper;            // 整数展开：16 -> 32

	int64_t _stat_ts;    // 时间戳
	int64_t _max_id;     // 最大序号

	int64_t _k_loss_stats_window_ms;	// 最大时间窗口
	int64_t _k_max_stats_window_num;	// 最大丢包序号
	int _k_calculation_limit;			// 最少多少个包开始统计
};



//---------------------------------------------------------------------
// 收包速率统计
//---------------------------------------------------------------------
class AbstractRateStats
{
public:
	virtual ~AbstractRateStats();
	AbstractRateStats();
	AbstractRateStats(const AbstractRateStats& src);
	AbstractRateStats(AbstractRateStats&& src);

	AbstractRateStats& operator = (const AbstractRateStats& src);

public:

	// 初始化：wnd_size 是窗口大小，scale 是速率的单位，比如 8000 代表速率单位是 kbps
	// wnd_size 和 scale 传负数表示不变，继续使用之前的值
	// (2000, 8000) 是比较合理的默认值，表示统计最近 2 秒的速率，单位是 kbps
	// (2000, 1000) 也是比较合理的默认值，表示统计最近 2 秒的速率，单位是 Bytes/s
	void init(int wnd_size, float scale);

	// 复位
	void reset();

	// 收到一个包时调用
	void update(size_t count, int64_t now_ts);

	// 统计码率，数据不够返回 -1，成功返回速率
	int calculate(int64_t now_ts);

	// 取得积累值
	int64_t accumulated() const { return _accumulated_count; }

	// 取得样本数
	int samples() const { return _sample_num; }

protected:
	void evict_oldest(int64_t now_ts);

protected:
	struct RateBucket { int sum, sample; };
	std::vector<RateBucket> _buckets;

protected:
	int64_t _oldest_ts;
	int _oldest_index;
	int _wnd_size;
	float _scale;
	int64_t _accumulated_count;
	int _sample_num;
};


//---------------------------------------------------------------------
// Minimum Sliding Window
//---------------------------------------------------------------------
class MinHistory
{
public:
	virtual ~MinHistory();
	MinHistory();
	MinHistory(const MinHistory& src);
	MinHistory(MinHistory&& src);

	MinHistory& operator = (const MinHistory& src);

public:

	// reverse 为假时求最小值，否则求最大值
	void init(int wnd_size, bool reverse = false);

	// clear history
	void clear();

	// update value
	void update(int value, int64_t now_ts);

	// check if empty
	inline bool empty() const { return _history.empty(); }

	// get value
	inline int value() const { return empty()? 0 : _history.front().second; }

protected:
	std::deque<std::pair<int64_t, int>> _history;
	int _wnd_size = 2000;
	bool _reverse = false;
};


//---------------------------------------------------------------------
// 求时间窗口内的最大值
//---------------------------------------------------------------------
class MaxHistory
{
public:
	virtual ~MaxHistory();
	MaxHistory();
	MaxHistory(const MaxHistory& src);
	MaxHistory(MaxHistory&& src);

	MaxHistory& operator = (const MaxHistory& src);

public:
	void init(int wnd_size);

	void clear();

	void update(int value, int64_t now_ts);

	inline bool empty() const { return _min_history.empty(); }

	inline int value() const { return _min_history.value(); }

protected:
	MinHistory _min_history;
};


//---------------------------------------------------------------------
// 平均值
//---------------------------------------------------------------------
class MovingAverage
{
public:
	virtual ~MovingAverage();
	MovingAverage();

public:
	void init(int wnd_size);

	void clear();

	void update(int value, int64_t now_ts);

	inline bool empty() const { return _history.empty(); }
	
	inline int value() const { return _average; }

	inline int count() const { return (int)_history.size(); }

protected:
	std::deque<std::pair<int64_t, int>> _history;
	int64_t _sum;
	int _average;
	int _wnd_size;
};


//---------------------------------------------------------------------
// RttHistory
//---------------------------------------------------------------------
class RttHistory
{
public:
	virtual ~RttHistory();
	RttHistory();

public:

	// initialize, wnd_size is the number of samples to keep
	void init(int wnd_size);

	// clear history
	void clear();

	// push a new rtt sample
	void push(double rtt);

	inline int count() const { return (int)_samples.size(); }
	inline bool empty() const { return count() == 0; }

	inline double rto() const { return _rto; }
	inline double avg() const { return _avg; }
	inline double deviation() const { return _deviation; }
	inline double minimum() const { return _min; }
	inline double maximum() const { return _max; }
	inline double jitter() const { return _jitter; }

private:
	std::deque<double> _samples;
	int _wnd_size;
	double _sum;
	double _avg;
	double _rto;
	double _srtt;
	double _rttval;
	double _deviation;
	double _min;
	double _max;
	double _jitter;
};


//---------------------------------------------------------------------
// 带宽预算（Token Bucket）
//---------------------------------------------------------------------
class PacingBudget
{
public:
	PacingBudget();

public:

	// 初始化：bandwidth 为带宽（bytes/sec），burst 默认等于 bandwidth
	void init(int64_t bandwidth_bps);

	// 重置状态
	void reset();

	// 查询当前可发送字节数（内部自动更新时间，累加预算）
	int64_t available(int64_t now_ms);

	// 登记实际发送的字节数，扣减预算
	void consume(int64_t bytes_sent);

	// 计算发送 bytes_needed 还需等多少毫秒（已够返回 0）
	int64_t wait_time(int64_t bytes_needed) const;

	// 取得当前预算
	inline int64_t budget() const { return _budget; }

	// 取得带宽设置
	inline int64_t bandwidth() const { return _bandwidth; }

protected:
	int64_t _bandwidth;    // bytes per second
	int64_t _burst;        // 最大累积预算（字节）
	int64_t _budget;       // 当前可用预算（字节），可为负数
	int64_t _last_time;    // 上次更新的时间戳（毫秒）
	int64_t _remainder;    // 累积余数（微秒级精度补偿）
};



//---------------------------------------------------------------------
// namespace end
//---------------------------------------------------------------------
NAMESPACE_END(System);



