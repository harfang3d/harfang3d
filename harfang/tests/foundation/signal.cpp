// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/signal.h"

#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/string.h"

using namespace hg;

template <typename T> struct ListenerObject {
	ListenerObject(int _id) : id(_id) {}
	void OnSignal(T t) { log(format("Instance #%1 received signal: %2").arg(id).arg(t)); }
	int id;
};

static void test_SignalToInstance() {
	Signal<void(std::string)> signal;

	ListenerObject<std::string> listener_a(0), listener_b(1);

	auto a = signal.Connect(std::bind(&ListenerObject<std::string>::OnSignal, &listener_a, std::placeholders::_1));
	TEST_CHECK(signal.GetListenerCount() == 1);
	auto b = signal.Connect(std::bind(&ListenerObject<std::string>::OnSignal, &listener_b, std::placeholders::_1));
	TEST_CHECK(signal.GetListenerCount() == 2);

	signal.Emit("Hello signal!");

	signal.Disconnect(a);
	TEST_CHECK(signal.GetListenerCount() == 1);
	signal.Disconnect(b);
	TEST_CHECK(signal.GetListenerCount() == 0);
}

//
template <typename T> void StaticListener(T t) { log(format("Static listener received signal: %1").arg(t)); }

static void test_SignalToFunction() {
	Signal<void(std::string)> signal;

	auto id = signal.Connect(&StaticListener<std::string>);
	TEST_CHECK(signal.GetListenerCount() == 1);

	signal.Emit("Hello signal!");

	signal.Disconnect(id);
	TEST_CHECK(signal.GetListenerCount() == 0);
}

//
struct CostlyCopy {
	std::string s[8];
};

static void RefListener(const CostlyCopy &s) {
	log("Reference listener received a signal:");
	for (int n = 0; n < 8; ++n)
		log(format("[%1]: %2").arg(n).arg(s.s[n]));
}

static void test_SignalByRef() {
	Signal<void(const CostlyCopy &)> signal;

	auto id = signal.Connect(&RefListener);
	TEST_CHECK(signal.GetListenerCount() == 1);

	CostlyCopy hs;
	for (int n = 0; n < 8; ++n)
		hs.s[n] = std::string("This is the string #") + std::to_string(n);

	signal.Emit(hs);

	signal.Disconnect(id);
	TEST_CHECK(signal.GetListenerCount() == 0);
}

//
struct VoidListenerObject {
	VoidListenerObject(int _id) : id(_id) {}
	void OnSignal() { log(format("Instance #%1 received void signal").arg(id)); }
	int id;
};

static void test_VoidSignalToInstance() {
	Signal<void()> signal;

	VoidListenerObject listener_a(0), listener_b(1);

	auto a = signal.Connect(std::bind(&VoidListenerObject::OnSignal, &listener_a));
	TEST_CHECK(signal.GetListenerCount() == 1);
	auto b = signal.Connect(std::bind(&VoidListenerObject::OnSignal, &listener_b));
	TEST_CHECK(signal.GetListenerCount() == 2);

	signal.Emit();

	signal.Disconnect(a);
	TEST_CHECK(signal.GetListenerCount() == 1);
	signal.Disconnect(b);
	TEST_CHECK(signal.GetListenerCount() == 0);
}

//
static void StaticVoidListener() { log("Void listener received signal"); }

static void test_VoidSignalToFunction() {
	Signal<void()> signal;

	auto id = signal.Connect(&StaticVoidListener);
	TEST_CHECK(signal.GetListenerCount() == 1);

	signal.Emit();

	signal.Disconnect(id);
	TEST_CHECK(signal.GetListenerCount() == 0);
}

//
struct BaseListenerObject {
	virtual ~BaseListenerObject() {}
	virtual void OnSignal() = 0;
};

struct InheritedListenerObject : public BaseListenerObject {
	InheritedListenerObject() : received(false) {}

	void OnSignal() override {
		log("Virtual member received void signal");
		received = true;
	}

	bool received;
};

static void test_SignalThroughInstanceVirtualMember() {
	Signal<void()> signal;

	std::unique_ptr<BaseListenerObject> listener(new InheritedListenerObject);

	auto id = signal.Connect(std::bind(&BaseListenerObject::OnSignal, listener.get()));
	TEST_CHECK(signal.GetListenerCount() == 1);

	signal.Emit();
	TEST_CHECK(static_cast<InheritedListenerObject *>(listener.get())->received);

	signal.Disconnect(id);
	TEST_CHECK(signal.GetListenerCount() == 0);
}

//
static void test_VoidSignalController() {
	Signal<void()> signal;

	bool signal_called = false;
	signal.Connect([&signal_called]() { signal_called = true; });

	bool controller_called = false;
	signal.Emit([&controller_called]() { controller_called = true; });

	TEST_CHECK(signal_called);
	TEST_CHECK(controller_called);
}

static void test_SignalController() {
	Signal<int()> signal;

	bool signal_called = false;
	signal.Connect([&signal_called]() -> int {
		signal_called = true;
		return 66;
	});

	int controller_result = 0;
	signal.Emit([&controller_result](int v) { controller_result = v; });

	TEST_CHECK(signal_called);
	TEST_CHECK(controller_result == 66);
}

void test_signal() {
	test_SignalToInstance();
	test_SignalToFunction();
	test_SignalByRef();
	test_VoidSignalToInstance();
	test_VoidSignalToFunction();
	test_SignalThroughInstanceVirtualMember();
	test_VoidSignalController();
	test_SignalController();
}