// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include <string>
#include <iostream>
#include <sstream> 

#include "foundation/log.h"
#include "foundation/string.h"

using namespace hg;

struct Output {
	std::string msg;
	std::string details;
	int mask;
};

void clear(Output &out) {
	out.msg.clear();
	out.details.clear();
	out.mask = 0;
}

static void on_log(const char* msg, int mask, const char* details, void* user) { 
	Output *out = reinterpret_cast<Output *>(user);
	out->msg = msg ? msg : "";
	out->details = details ? details : "";
	out->mask = mask;
}

void clear(std::stringstream &buffer) {
	buffer.clear();
	buffer.str("");
}

void test_log() {
	set_log_hook(nullptr, nullptr);

	set_log_detailed(false);
	TEST_CHECK(get_log_detailed() == false);

	set_log_detailed(true);
	TEST_CHECK(get_log_detailed() == true);

	set_log_level(LL_Normal);
	TEST_CHECK(get_log_level() == LL_Normal);

	set_log_level(LL_Normal | LL_Warning);
	TEST_CHECK(get_log_level() == (LL_Normal | LL_Warning));

	std::stringstream buffer;
	std::streambuf *prev = std::cout.rdbuf(buffer.rdbuf());

	set_log_level(LL_All);

	log("log 0000");
	TEST_CHECK(contains(buffer.str(), "log 0000") == true);
	TEST_CHECK(contains(buffer.str(), "ERROR:") == false);
	TEST_CHECK(contains(buffer.str(), "WARNING:") == false);
	TEST_CHECK(contains(buffer.str(), "DEBUG:") == false);
	TEST_CHECK(contains(buffer.str(), "Details:") == false);
	clear(buffer);
	
	log("log 0001", "details");
	TEST_CHECK(contains(buffer.str(), "log 0001") == true);
	TEST_CHECK(contains(buffer.str(), "ERROR:") == false);
	TEST_CHECK(contains(buffer.str(), "WARNING:") == false);
	TEST_CHECK(contains(buffer.str(), "DEBUG:") == false);
	TEST_CHECK(contains(buffer.str(), "Details:") == true);
	clear(buffer);
	
	set_log_detailed(false);

	log("log 0001", "details");
	TEST_CHECK(contains(buffer.str(), "Details:") == false);
	clear(buffer);

	set_log_detailed(true);
	set_log_level(LL_Warning | LL_Debug | LL_Error);

	log("log 0002", "details");
	TEST_CHECK(buffer.str().empty());
	clear(buffer);

	warn("warn 0000");
	TEST_CHECK(contains(buffer.str(), "warn 0000") == true);
	TEST_CHECK(contains(buffer.str(), "ERROR:") == false);
	TEST_CHECK(contains(buffer.str(), "WARNING:") == true);
	TEST_CHECK(contains(buffer.str(), "DEBUG:") == false);
	TEST_CHECK(contains(buffer.str(), "Details:") == false);
	clear(buffer);

	warn("warn 0001", "details");
	TEST_CHECK(contains(buffer.str(), "warn 0001") == true);
	TEST_CHECK(contains(buffer.str(), "ERROR:") == false);
	TEST_CHECK(contains(buffer.str(), "WARNING:") == true);
	TEST_CHECK(contains(buffer.str(), "DEBUG:") == false);
	TEST_CHECK(contains(buffer.str(), "Details:") == true);
	clear(buffer);

	set_log_detailed(false);
	warn("warn 0002", "details");
	TEST_CHECK(contains(buffer.str(), "Details:") == false);
	clear(buffer);

	set_log_detailed(true);
	error("error 0000");
	TEST_CHECK(contains(buffer.str(), "error 0000") == true);
	TEST_CHECK(contains(buffer.str(), "ERROR:") == true);
	TEST_CHECK(contains(buffer.str(), "WARNING:") == false);
	TEST_CHECK(contains(buffer.str(), "DEBUG:") == false);
	TEST_CHECK(contains(buffer.str(), "Details:") == false);
	clear(buffer);

	error("error 0001", "details");
	TEST_CHECK(contains(buffer.str(), "error 0001") == true);
	TEST_CHECK(contains(buffer.str(), "ERROR:") == true);
	TEST_CHECK(contains(buffer.str(), "WARNING:") == false);
	TEST_CHECK(contains(buffer.str(), "DEBUG:") == false);
	TEST_CHECK(contains(buffer.str(), "Details:") == true);
	clear(buffer);

	set_log_detailed(false);
	error("error 0002", "details");
	TEST_CHECK(contains(buffer.str(), "Details:") == false);
	clear(buffer);

	set_log_detailed(true);
	debug("debug 0000");
	TEST_CHECK(contains(buffer.str(), "debug 0000") == true);
	TEST_CHECK(contains(buffer.str(), "ERROR:") == false);
	TEST_CHECK(contains(buffer.str(), "WARNING:") == false);
	TEST_CHECK(contains(buffer.str(), "DEBUG:") == true);
	TEST_CHECK(contains(buffer.str(), "Details:") == false);
	clear(buffer);

	debug("debug 0001", "details");
	TEST_CHECK(contains(buffer.str(), "debug 0001") == true);
	TEST_CHECK(contains(buffer.str(), "ERROR:") == false);
	TEST_CHECK(contains(buffer.str(), "WARNING:") == false);
	TEST_CHECK(contains(buffer.str(), "DEBUG:") == true);
	TEST_CHECK(contains(buffer.str(), "Details:") == true);
	clear(buffer);

	set_log_detailed(false);
	debug("debug 0002", "details");
	TEST_CHECK(contains(buffer.str(), "Details:") == false);
	clear(buffer);

	set_log_level(LL_Warning | LL_Error);
	set_log_detailed(true);

	debug("debug 0003");
	TEST_CHECK(buffer.str().empty());

	debug("debug 0004", "details");
	TEST_CHECK(buffer.str().empty());

	warn("warn 0003", "details");
	TEST_CHECK(contains(buffer.str(), "WARNING:") == true);

	std::cout.rdbuf(prev);

	Output out;
	set_log_hook(on_log, &out);

	clear(out);
	log("log 0000");
	TEST_CHECK(out.msg == "log 0000");
	TEST_CHECK(out.details.empty());
	TEST_CHECK(out.mask == LL_Normal);

	log("log 0001", "details");
	TEST_CHECK(out.msg == "log 0001");
	TEST_CHECK(out.details == "details");
	TEST_CHECK(out.mask == LL_Normal);

	warn("warn 0000");
	TEST_CHECK(out.msg == "warn 0000");
	TEST_CHECK(out.details.empty());
	TEST_CHECK(out.mask == LL_Warning);

	warn("warn 0001", "details");
	TEST_CHECK(out.msg == "warn 0001");
	TEST_CHECK(out.details == "details");
	TEST_CHECK(out.mask == LL_Warning);

	error("error 0000");
	TEST_CHECK(out.msg == "error 0000");
	TEST_CHECK(out.details.empty());
	TEST_CHECK(out.mask == LL_Error);

	error("error 0001", "details");
	TEST_CHECK(out.msg == "error 0001");
	TEST_CHECK(out.details == "details");
	TEST_CHECK(out.mask == LL_Error);

	debug("debug 0000");
	TEST_CHECK(out.msg == "debug 0000");
	TEST_CHECK(out.details.empty());
	TEST_CHECK(out.mask == LL_Debug);

	debug("debug 0001", "details");
	TEST_CHECK(out.msg == "debug 0001");
	TEST_CHECK(out.details == "details");
	TEST_CHECK(out.mask == LL_Debug);

	set_log_hook(nullptr, nullptr);
}