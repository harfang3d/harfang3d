// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#if 0

#if _WIN32
#include <Windows.h>

struct StartWSA {
	StartWSA()
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
	}
};

static StartWSA start_wsa;
#endif

#include "gtest/gtest.h"
#include "http_happy/http_happy.h"
#include "foundation/vector.h"
#include "foundation/thread.h"

using namespace hg;

static uint g_received_request_id = 0xffffffff;

static void RequestCompleteCallback(uint request_id, uint http_status, const ByteArray &data) { g_received_request_id = request_id; }

TEST(HTTP, PostToGoogle) {
	http::RegisterRequestCompleteCallback(&RequestCompleteCallback, nullptr);

	uint id = http::Get("google.com", "/");
	while (g_received_request_id == 0xffffffff)
		std::this_thread::sleep_for(std::chrono::milliseconds(5));

	http::UnregisterRequestCompleteCallback(&RequestCompleteCallback);

	EXPECT_EQ(id, g_received_request_id);
}

TEST(HTTP, PostToDummy) {
//	uint id = http::Get("12.41.12.512", "/");
}

#endif
