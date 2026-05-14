//=====================================================================
//
// AbstractStats.cpp - 
//
// Last Modified: 2020/04/03 17:47:58
//
//=====================================================================
#include <assert.h>
#include <limits>

#include "netstats.h"



//---------------------------------------------------------------------
// Namespace begin
//---------------------------------------------------------------------
NAMESPACE_BEGIN(System);


//=====================================================================
// unsined integer unwrapper
//=====================================================================

UnsignedWrap::UnsignedWrap() 
{
	size = 16;
	last_value = -1;
}

UnsignedWrap::UnsignedWrap(int size)
{
	this->size = 16;
	last_value = -1;
	reset(size);
}

UnsignedWrap::UnsignedWrap(const UnsignedWrap& src)
{
	size = src.size;
	last_value = src.last_value;
}

UnsignedWrap& UnsignedWrap::operator = (const UnsignedWrap& src)
{
	size = src.size;
	last_value = src.last_value;
	return *this;
}

void UnsignedWrap::reset(int size)
{
	if (size == 16 || size == 2) {
		this->size = 16;
	}
	else if (size == 32 || size == 4) {
		this->size = 32;
	}
	else {
		this->size = size;
	}
	assert(size == 16 || size == 2 || size == 32 || size == 4);
	last_value = -1;
}

void UnsignedWrap::set_last(int64_t last)
{
	last_value = last;
}

bool UnsignedWrap::u16_is_newer(uint16_t val, uint16_t prev_val)
{
	const uint16_t half = ((uint16_t)0x8000);
	if (val - prev_val == half)
		return (val > prev_val)? true : false;
	if (val != prev_val && ((uint16_t)(val - prev_val)) < half)
		return true;
	return false;
}

bool UnsignedWrap::u32_is_newer(uint32_t val, uint32_t prev_val)
{
	const uint32_t half = ((uint32_t)0x80000000);
	if (val - prev_val == half)
		return (val > prev_val)? true : false;
	if (val != prev_val && ((uint32_t)(val - prev_val)) < half)
		return true;
	return false;
}

int64_t UnsignedWrap::u16_update(uint16_t val)
{
	const int64_t max_plus = ((int64_t)0xffff) + 1;
	uint16_t cropped_last = (uint16_t)(last_value & 0xffff);
	int64_t delta = ((int64_t)val) - ((int64_t)cropped_last);
	if (u16_is_newer(val, cropped_last)) {
		if (delta < 0) {
			delta += max_plus;
		}
	}
	else if (delta > 0 && last_value + delta - max_plus >= 0) {
		delta -= max_plus;
	}
	return last_value + delta;
}

int64_t UnsignedWrap::u32_update(uint32_t val)
{
	const int64_t max_plus = ((int64_t)0xffffffff) + 1;
	uint32_t cropped_last = (uint32_t)(last_value & 0xffffffff);
	int64_t delta = ((int64_t)val) - ((int64_t)cropped_last);
	if (u32_is_newer(val, cropped_last)) {
		if (delta < 0) {
			delta += max_plus;
		}
	}
	else if (delta > 0 && last_value + delta - max_plus >= 0) {
		delta -= max_plus;
	}
	return last_value + delta;
}

int64_t UnsignedWrap::wrap_uint16(uint16_t val)
{
	assert(size == 16);
	if (last_value < 0) {
		last_value = val;
	}	else {
		last_value = u16_update(val);
	}
	return last_value;
}

int64_t UnsignedWrap::wrap_uint32(uint32_t val)
{
	assert(size == 32);
	if (last_value < 0) {
		last_value = val;
	}	else {
		last_value = u32_update(val);
	}
	return last_value;
}


//=====================================================================
// 丢包统计
//=====================================================================

//---------------------------------------------------------------------
// ctor
//---------------------------------------------------------------------
AbstractLossStats::AbstractLossStats()
{
	init(4000, 200, 100);
}


//---------------------------------------------------------------------
// dtor
//---------------------------------------------------------------------
AbstractLossStats::~AbstractLossStats()
{
}


//---------------------------------------------------------------------
// copy ctor
//---------------------------------------------------------------------
AbstractLossStats::AbstractLossStats(const AbstractLossStats& src):
	_loss_window(src._loss_window),
	_wrapper(src._wrapper),
	_stat_ts(src._stat_ts),
	_max_id(src._max_id),
	_k_loss_stats_window_ms(src._k_loss_stats_window_ms),
	_k_max_stats_window_num(src._k_max_stats_window_num),
	_k_calculation_limit(src._k_calculation_limit)
{
}


//---------------------------------------------------------------------
// move ctor
//---------------------------------------------------------------------
AbstractLossStats::AbstractLossStats(AbstractLossStats&& src):
	_loss_window(std::move(src._loss_window)),
	_wrapper(src._wrapper),
	_stat_ts(src._stat_ts),
	_max_id(src._max_id),
	_k_loss_stats_window_ms(src._k_loss_stats_window_ms),
	_k_max_stats_window_num(src._k_max_stats_window_num),
	_k_calculation_limit(src._k_calculation_limit)
{
}


//---------------------------------------------------------------------
// copy assignment
//---------------------------------------------------------------------
AbstractLossStats& AbstractLossStats::operator = (const AbstractLossStats& src)
{
	_loss_window.clear();
	for (auto it: src._loss_window) {
		_loss_window[it.first] = it.second;
	}
	_wrapper = src._wrapper;
	_stat_ts = src._stat_ts;
	_max_id = src._max_id;
	_k_loss_stats_window_ms = src._k_loss_stats_window_ms;
	_k_max_stats_window_num = src._k_max_stats_window_num;
	_k_calculation_limit = src._k_calculation_limit;
	return *this;
}


//---------------------------------------------------------------------
// 初始化统计
//---------------------------------------------------------------------
void AbstractLossStats::init(int window_ms, int window_num, int limit)
{
	if (limit < 10) limit = 10;
	if (window_num <= limit) window_num = limit + 1;
	_k_loss_stats_window_ms = window_ms;
	_k_max_stats_window_num = window_num;
	_k_calculation_limit = limit;
	_stat_ts = -1;
	_max_id = -1;
	_loss_window.clear();
	_wrapper.reset(16);
}


//---------------------------------------------------------------------
// 复位统计
//---------------------------------------------------------------------
void AbstractLossStats::reset()
{
	_stat_ts = -1;
	_max_id = -1;
	_loss_window.clear();
	_wrapper.reset(16);
}


//---------------------------------------------------------------------
// 淘汰超过窗口的太老的数据
//---------------------------------------------------------------------
void AbstractLossStats::evict_oldest(int64_t now_ts)
{
	while (_loss_window.size() > 0) {
		LossWindow::iterator it = _loss_window.begin();
		bool drop = false;
		if ((int)_loss_window.size() > _k_max_stats_window_num) {
			drop = true;
		}
		else if (it->second + _k_loss_stats_window_ms < now_ts) {
			drop = true;
		}
		if (drop == false) {
			break;
		}
		else {
			_loss_window.erase(it);
		}
	}
}


//---------------------------------------------------------------------
// 收到一个包时调用
//---------------------------------------------------------------------
void AbstractLossStats::update(uint16_t seq, int64_t now_ts)
{
	int64_t id = _wrapper.wrap_uint16(seq);
	if (_max_id < id) {
		_max_id = id;
	}
	_loss_window[id] = now_ts;
	evict_oldest(now_ts);
}


//---------------------------------------------------------------------
// 计算丢包率：fraction_loss 是返回的小数丢包率，255 代表 100%
//---------------------------------------------------------------------
int AbstractLossStats::calculate(int64_t now_ts, uint8_t *fraction_loss, int *num)
{
	*fraction_loss = 0;
	*num = 0;

	evict_oldest(now_ts);

	if (_max_id < 0) {
		return -1;
	}

	if ((int)_loss_window.size() < _k_calculation_limit) {
		// printf("not enough: size=%d limit=%d\n", (int)_loss_window.size(), _k_calculation_limit);
		return -2;
	}

	int count = (int)_loss_window.size();

	if (count <= 0) {
		return -3;
	}

	auto it = _loss_window.begin();
	int64_t oldest = it->first;

	int distance = (int32_t)(_max_id - oldest + 1);
	if (distance <= 0)
		return -4;

	if (distance <= count) {
		*fraction_loss = 0;
	}
	else {
		*fraction_loss = (distance - count) * 255 / distance;
	}

	*num = distance;
	_stat_ts = now_ts;

	return 0;
}



//=====================================================================
// 速率统计
//=====================================================================


//---------------------------------------------------------------------
// ctor
//---------------------------------------------------------------------
AbstractRateStats::AbstractRateStats()
{
	_wnd_size = 2000;
	_scale = 8000;
	init(2000, 8000);
}


//---------------------------------------------------------------------
// dtor
//---------------------------------------------------------------------
AbstractRateStats::~AbstractRateStats()
{
}


//---------------------------------------------------------------------
// copy ctor
//---------------------------------------------------------------------
AbstractRateStats::AbstractRateStats(const AbstractRateStats& src):
	_buckets(src._buckets),
	_oldest_ts(src._oldest_ts),
	_oldest_index(src._oldest_index),
	_wnd_size(src._wnd_size),
	_scale(src._scale),
	_accumulated_count(src._accumulated_count),
	_sample_num(src._sample_num)
{
}


//---------------------------------------------------------------------
// move ctor
//---------------------------------------------------------------------
AbstractRateStats::AbstractRateStats(AbstractRateStats&& src):
	_buckets(std::move(src._buckets)),
	_oldest_ts(src._oldest_ts),
	_oldest_index(src._oldest_index),
	_wnd_size(src._wnd_size),
	_scale(src._scale),
	_accumulated_count(src._accumulated_count),
	_sample_num(src._sample_num)
{
}


//---------------------------------------------------------------------
// copy assign
//---------------------------------------------------------------------
AbstractRateStats& AbstractRateStats::operator = (const AbstractRateStats& src)
{
	_buckets = src._buckets;
	_oldest_ts = src._oldest_ts;
	_oldest_index = src._oldest_index;
	_wnd_size = src._wnd_size;
	_scale = src._scale;
	_accumulated_count = src._accumulated_count;
	_sample_num = src._sample_num;
	return *this;
}


//---------------------------------------------------------------------
// initialize
//---------------------------------------------------------------------
void AbstractRateStats::init(int wnd_size, float scale)
{
	if (wnd_size < 0) wnd_size = _wnd_size;
	if (scale < 0) scale = _scale;
	_wnd_size = wnd_size;
	_scale = scale;
	_buckets.resize(wnd_size);
	reset();
}


//---------------------------------------------------------------------
// reset
//---------------------------------------------------------------------
void AbstractRateStats::reset()
{
	_buckets.resize(_wnd_size);
	_accumulated_count = 0;
	_sample_num = 0;
	_oldest_index = 0;
	_oldest_ts = -1;
	for (int i = 0; i < _wnd_size; i++) {
		_buckets[i].sum = 0;
		_buckets[i].sample = 0;
	}
}


//---------------------------------------------------------------------
// 删除过期数据
//---------------------------------------------------------------------
void AbstractRateStats::evict_oldest(int64_t now_ts)
{
	if (_oldest_ts < 0) 
		return;

	int64_t new_oldest_ts = now_ts - _wnd_size + 1;

	if (new_oldest_ts < _oldest_ts)
		return;

	while (_sample_num > 0 && _oldest_ts < new_oldest_ts) {
		RateBucket& bucket = _buckets[_oldest_index];
		_sample_num -= bucket.sample;
		_accumulated_count -= bucket.sum;
		bucket.sum = 0;
		bucket.sample = 0;

		if (++_oldest_index >= _wnd_size) {
			_oldest_index = 0;
		}

		_oldest_ts++;
	}

	_oldest_ts = new_oldest_ts;
}


//---------------------------------------------------------------------
// 收到一个包时调用
//---------------------------------------------------------------------
void AbstractRateStats::update(size_t count, int64_t now_ts)
{
	if (_oldest_ts > now_ts)
		return;

	evict_oldest(now_ts);

	if (_oldest_ts < 0) {
		_oldest_ts = now_ts;
	}

	int offset = (int)(now_ts - _oldest_ts);
	int index = (_oldest_index + offset) % _wnd_size;

	_sample_num++;
	_buckets[index].sum += (int)count;
	_buckets[index].sample++;

	_accumulated_count += count;
}


//---------------------------------------------------------------------
// 统计码率，数据不够返回 -1，成功返回速率
//---------------------------------------------------------------------
int AbstractRateStats::calculate(int64_t now_ts)
{
	evict_oldest(now_ts);

	int active_wnd_size = (int)((int64_t)(now_ts - _oldest_ts + 1));

	if (_sample_num <= 0 || active_wnd_size <= 1 || active_wnd_size < _wnd_size)
		return -1;

	double rate = ((((double)_accumulated_count) * _scale) / _wnd_size) + 0.5;

	return (int)rate;
}


//---------------------------------------------------------------------
// Minimum Sliding Window
//---------------------------------------------------------------------
MinHistory::MinHistory()
{
	init(4000, false);
}


//---------------------------------------------------------------------
// dtor
//---------------------------------------------------------------------
MinHistory::~MinHistory()
{
}


//---------------------------------------------------------------------
// copy ctor
//---------------------------------------------------------------------
MinHistory::MinHistory(const MinHistory& src)
{
	this->operator=(src);
}


//---------------------------------------------------------------------
// moving ctor
//---------------------------------------------------------------------
MinHistory::MinHistory(MinHistory&& src):
	_history(std::move(src._history)),
	_wnd_size(src._wnd_size),
	_reverse(src._reverse)
{
}


//---------------------------------------------------------------------
// copy assignment
//---------------------------------------------------------------------
MinHistory& MinHistory::operator = (const MinHistory& src)
{
	_history.clear();
	for (auto &x : src._history) {
		_history.push_back(x);
	}
	_wnd_size = src._wnd_size;
	_reverse = src._reverse;
	return *this;
}


//---------------------------------------------------------------------
// mode: 0 for minimal, 1 for maximal, win size in millisecs
//---------------------------------------------------------------------
void MinHistory::init(int wnd_size, bool reverse)
{
	_wnd_size = wnd_size;
	_reverse = reverse;
	_history.clear();
}


//---------------------------------------------------------------------
// clear history
//---------------------------------------------------------------------
void MinHistory::clear()
{
	_history.clear();
}


//---------------------------------------------------------------------
// update value
//---------------------------------------------------------------------
void MinHistory::update(int value, int64_t now_ts)
{
	while (!_history.empty()) {
		if (now_ts - _history.front().first + 1 <= _wnd_size) break;
		_history.pop_front();
	}
	while (!_history.empty()) {
		if (_reverse == false) {
			if (_history.back().second < value) break;
		}
		else {
			if (_history.back().second > value) break;
		}
		_history.pop_back();
	}
	_history.push_back(std::make_pair(now_ts, value));
}


//---------------------------------------------------------------------
// ctor
//---------------------------------------------------------------------
MaxHistory::MaxHistory()
{
	_min_history.init(4000, true);
}


//---------------------------------------------------------------------
// dtor
//---------------------------------------------------------------------
MaxHistory::~MaxHistory()
{
}


//---------------------------------------------------------------------
// copy ctor
//---------------------------------------------------------------------
MaxHistory::MaxHistory(const MaxHistory& src)
{
	_min_history = src._min_history;
}


//---------------------------------------------------------------------
// move ctor
//---------------------------------------------------------------------
MaxHistory::MaxHistory(MaxHistory&& src):
	_min_history(std::move(src._min_history))
{
}


//---------------------------------------------------------------------
// copy assignment
//---------------------------------------------------------------------
MaxHistory& MaxHistory::operator=(const MaxHistory &src)
{
	_min_history = src._min_history;
	return *this;
}



//---------------------------------------------------------------------
// init
//---------------------------------------------------------------------
void MaxHistory::init(int wnd_size)
{
	_min_history.init(wnd_size, true);
}


//---------------------------------------------------------------------
// clear
//---------------------------------------------------------------------
void MaxHistory::clear()
{
	_min_history.clear();
}


//---------------------------------------------------------------------
// update
//---------------------------------------------------------------------
void MaxHistory::update(int value, int64_t now_ts)
{
	_min_history.update(value, now_ts);
}


//---------------------------------------------------------------------
// ctor
//---------------------------------------------------------------------
MovingAverage::MovingAverage()
{
	_wnd_size = 5000;
	_sum = 0;
	_average = 0;
}


//---------------------------------------------------------------------
// dtor
//---------------------------------------------------------------------
MovingAverage::~MovingAverage()
{
}


//---------------------------------------------------------------------
// reset
//---------------------------------------------------------------------
void MovingAverage::init(int wnd_size)
{
	_wnd_size = wnd_size;
}
	

//---------------------------------------------------------------------
// clear
//---------------------------------------------------------------------
void MovingAverage::clear()
{
	_history.clear();
	_sum = 0;
	_average = 0;
}


//---------------------------------------------------------------------
// update value
//---------------------------------------------------------------------
void MovingAverage::update(int value, int64_t now_ts)
{
	while (!_history.empty()) {
		if (now_ts - _history.front().first + 1 <= _wnd_size) break;
		_sum -= _history.front().second;
		_history.pop_front();
	}
	_history.push_back(std::make_pair(now_ts, value));
	_sum += value;
	_average = (int)(_sum / _history.size());
}


//---------------------------------------------------------------------
// ctor
//---------------------------------------------------------------------
RttHistory::RttHistory()
{
	init(200);
}


//---------------------------------------------------------------------
// dtor
//---------------------------------------------------------------------
RttHistory::~RttHistory()
{
}


//---------------------------------------------------------------------
// initialize, wnd_size is the number of samples to keep
//---------------------------------------------------------------------
void RttHistory::init(int wnd_size)
{
	_wnd_size = wnd_size;
	_sum = 0;
	_avg = 0;
	_srtt = -1;
	_rttval = -1;
	_rto = 0;
	_deviation = 0;
	_min = 0;
	_max = 0;
	_jitter = 0;
	_samples.clear();
}


//---------------------------------------------------------------------
// clear history
//---------------------------------------------------------------------
void RttHistory::clear()
{
	_samples.clear();
}


//---------------------------------------------------------------------
// push a new rtt sample
//---------------------------------------------------------------------
void RttHistory::push(double rtt)
{
	_samples.push_back(rtt);
	_sum += rtt;
	if ((int)_samples.size() > _wnd_size) {
		_sum -= _samples.front();
		_samples.pop_front();
	}
	if (_samples.size() > 0) {
		_avg = _sum / _samples.size();
		double sum = 0;
		double minimum = 0;
		double maximum = 0;
		bool first = true;
		for (double rtt : _samples) {
			double diff = rtt - _avg;
			sum += diff * diff;
			if (first) {
				minimum = rtt;
				maximum = rtt;
				first = false;
			}
			if (rtt < minimum) minimum = rtt;
			if (rtt > maximum) maximum = rtt;
		}
		_deviation = sum / _samples.size();
		_min = minimum;
		_max = maximum;
		_jitter = (maximum - minimum) / 2;
	}
	if (_srtt < 0) {
		_srtt = rtt;
		_rttval = rtt / 2;
	}	else {
		double delta = rtt - _srtt;
		if (delta < 0) delta = -delta;
		_rttval = (3 * _rttval + delta) / 4;
		_srtt = (7 * _srtt + rtt) / 8;
		if (_srtt < 0) _srtt = 0;
	}
	_rto = _srtt + (_rttval < 0? 0 : (4 * _rttval));
}




//=====================================================================
// 带宽预算（Token Bucket）
//=====================================================================


//---------------------------------------------------------------------
// ctor
//---------------------------------------------------------------------
PacingBudget::PacingBudget()
{
	init(0);
}


//---------------------------------------------------------------------
// 初始化：bandwidth 为带宽（bytes/sec），burst 默认等于 bandwidth
//---------------------------------------------------------------------
void PacingBudget::init(int64_t bandwidth_bps)
{
	_bandwidth = bandwidth_bps;
	_burst = bandwidth_bps;
	_budget = 0;
	_last_time = -1;
	_remainder = 0;
}


//---------------------------------------------------------------------
// 重置状态
//---------------------------------------------------------------------
void PacingBudget::reset()
{
	_budget = 0;
	_last_time = -1;
	_remainder = 0;
}


//---------------------------------------------------------------------
// 查询当前可发送字节数（内部自动更新时间，累加预算）
//---------------------------------------------------------------------
int64_t PacingBudget::available(int64_t now_ms)
{
	if (_last_time < 0) {
		_last_time = now_ms;
		return _budget;
	}

	int64_t elapsed = now_ms - _last_time;

	if (elapsed <= 0) {
		return (_budget > 0) ? _budget : 0;
	}

	_last_time = now_ms;

	// 累加预算：bandwidth * elapsed / 1000，用 remainder 保留余数
	int64_t total = _bandwidth * elapsed + _remainder;
	int64_t increment = total / 1000;
	_remainder = total % 1000;

	_budget += increment;

	// 限制预算不超过 burst
	if (_budget > _burst) {
		_budget = _burst;
		_remainder = 0;
	}

	return (_budget > 0) ? _budget : 0;
}


//---------------------------------------------------------------------
// 登记实际发送的字节数，扣减预算
//---------------------------------------------------------------------
void PacingBudget::consume(int64_t bytes_sent)
{
	_budget -= bytes_sent;
}


//---------------------------------------------------------------------
// 计算发送 bytes_needed 还需等多少毫秒（已够返回 0）
//---------------------------------------------------------------------
int64_t PacingBudget::wait_time(int64_t bytes_needed) const
{
	int64_t deficit = bytes_needed - _budget;
	if (deficit <= 0) {
		return 0;
	}
	if (_bandwidth <= 0) {
		return -1;
	}
	return (deficit * 1000 + _bandwidth - 1) / _bandwidth;
}



//---------------------------------------------------------------------
// namespace end
//---------------------------------------------------------------------
NAMESPACE_END(System);



