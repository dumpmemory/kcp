//=====================================================================
//
// emulator.h - network emulator for testing KCP implementation
//
// Last Modified: 2026/05/14 15:50:26
//
//=====================================================================
#ifndef _EMULATOR_H_
#define _EMULATOR_H_

#include <stddef.h>
#include <stdint.h>

#include "../ikcp.h"
#include "netstats.h"
#include "netsim.h"

#include <functional>
#include <string>


//---------------------------------------------------------------------
// kcp
//---------------------------------------------------------------------
class Kcp
{
public:
	inline ~Kcp();
	inline Kcp(IUINT32 conv);

public:
	std::function<int(const char *buf, int len)> output;
	std::function<void(const char*)> writelog;

	inline ikcpcb* ptr() { return _kcp; }
	inline const ikcpcb* ptr() const { return _kcp; }

	inline int recv(char *buffer, int len);
	inline int recv(std::string &buffer);

	inline int send(const char *buffer, int len);
	inline int send(const std::string &buffer);

	inline void update(IUINT32 current);
	inline IUINT32 check(IUINT32 current) const;

	inline int input(const char *data, long size);
	inline void flush();

	inline int peeksize() const;
	inline int setmtu(int mtu);
	inline int wndsize(int sndwnd, int rcvwnd);
	inline int waitsnd() const;

	inline int nodelay(int nodelay, int interval, int resend, int nc);

	inline int setcc(const struct IKCPOPS *ops);

private:
	static int udp_output(const char *buf, int len, ikcpcb *kcp, void *user);
	static void kcp_writelog(const char *log, ikcpcb *kcp, void *user);

private:
	ikcpcb *_kcp;
};


//---------------------------------------------------------------------
// implementation
//---------------------------------------------------------------------

inline Kcp::~Kcp() {
	ikcp_release(_kcp);
}

inline Kcp::Kcp(IUINT32 conv) {
	_kcp = ikcp_create(conv, this);
	_kcp->output = udp_output;
}

inline int Kcp::udp_output(const char *buf, int len, ikcpcb *kcp, void *user) {
	Kcp *self = (Kcp *)user;
	if (self->output != nullptr) {
		return self->output(buf, len);
	}
	return -1;
}

inline void Kcp::kcp_writelog(const char *log, ikcpcb *kcp, void *user) {
	Kcp *self = (Kcp *)user;
	if (self->writelog != nullptr) {
		self->writelog(log);
	}
}

inline int Kcp::recv(char *buffer, int len) {
	return ikcp_recv(_kcp, buffer, len);
}

inline int Kcp::recv(std::string &buffer) {
	int peek = ikcp_peeksize(_kcp);
	if (peek < 0) {
		buffer.resize(0);
		return peek;
	}
	buffer.resize(peek);
	return ikcp_recv(_kcp, &buffer[0], peek);
}

inline int Kcp::send(const char *buffer, int len) {
	return ikcp_send(_kcp, buffer, len);
}

inline int Kcp::send(const std::string &buffer) {
	return ikcp_send(_kcp, buffer.c_str(), (int)buffer.size());
}


inline void Kcp::update(IUINT32 current) {
	ikcp_update(_kcp, current);
}

inline IUINT32 Kcp::check(IUINT32 current) const {
	return ikcp_check(_kcp, current);
}

inline int Kcp::input(const char *data, long size) {
	return ikcp_input(_kcp, data, size);
}

inline void Kcp::flush() {
	ikcp_flush(_kcp);
}

inline int Kcp::peeksize() const {
	return ikcp_peeksize(_kcp);
}

inline int Kcp::setmtu(int mtu) {
	return ikcp_setmtu(_kcp, mtu);
}

inline int Kcp::wndsize(int sndwnd, int rcvwnd) {
	return ikcp_wndsize(_kcp, sndwnd, rcvwnd);
}

inline int Kcp::waitsnd() const {
	return ikcp_waitsnd(_kcp);
}

inline int Kcp::nodelay(int nodelay, int interval, int resend, int nc) {
	return ikcp_nodelay(_kcp, nodelay, interval, resend, nc);
}

inline int Kcp::setcc(const struct IKCPOPS *ops) {
	return ikcp_setcc(_kcp, ops);
}


//---------------------------------------------------------------------
// KcpPacket
//---------------------------------------------------------------------
struct KcpPacket {
	int group;
	int direction;      // 0 for client->server, 1 for server->client
	std::string data;
};


#endif


