// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/signal.h"
#include "foundation/string.h"
#include "gtest/gtest.h"

using namespace hg;

template <typename T> struct ListenerObject {
	ListenerObject(int _id) : id(_id) {}
	void OnSignal(T t) { log(format("Instance #%1 received signal: %2").arg(id).arg(t)); }
	int id;
};

TEST(Signal, SignalToInstance) {
	Signal<void(std::string)> signal;

	ListenerObject<std::string> listener_a(0), listener_b(1);

	auto a = signal.Connect(std::bind(&ListenerObject<std::string>::OnSignal, &listener_a, std::placeholders::_1));
	EXPECT_EQ(1, signal.GetListenerCount());
	auto b = signal.Connect(std::bind(&ListenerObject<std::string>::OnSignal, &listener_b, std::placeholders::_1));
	EXPECT_EQ(2, signal.GetListenerCount());

	signal.Emit("Hello signal!");

	signal.Disconnect(a);
	EXPECT_EQ(1, signal.GetListenerCount());
	signal.Disconnect(b);
	EXPECT_EQ(0, signal.GetListenerCount());
}

//
template <typename T> void StaticListener(T t) { log(format("Static listener received signal: %1").arg(t)); }

TEST(Signal, SignalToFunction) {
	Signal<void(std::string)> signal;

	auto id = signal.Connect(&StaticListener<std::string>);
	EXPECT_EQ(1, signal.GetListenerCount());

	signal.Emit("Hello signal!");

	signal.Disconnect(id);
	EXPECT_EQ(0, signal.GetListenerCount());
}

//
struct CostlyCopy {
	std::string s[8];
};

void RefListener(const CostlyCopy &s) {
	log("Reference listener received a signal:");
	for (int n = 0; n < 8; ++n)
		log(format("[%1]: %2").arg(n).arg(s.s[n]));
}

TEST(Signal, SignalByRef) {
	Signal<void(const CostlyCopy &)> signal;

	auto id = signal.Connect(&RefListener);
	EXPECT_EQ(1, signal.GetListenerCount());

	CostlyCopy hs;
	for (int n = 0; n < 8; ++n)
		hs.s[n] = std::string("This is the string #") + std::to_string(n);

	signal.Emit(hs);

	signal.Disconnect(id);
	EXPECT_EQ(0, signal.GetListenerCount());
}

//
struct VoidListenerObject {
	VoidListenerObject(int _id) : id(_id) {}
	void OnSignal() { log(format("Instance #%1 received void signal").arg(id)); }
	int id;
};

TEST(Signal, VoidSignalToInstance) {
	Signal<void()> signal;

	VoidListenerObject listener_a(0), listener_b(1);

	auto a = signal.Connect(std::bind(&VoidListenerObject::OnSignal, &listener_a));
	EXPECT_EQ(1, signal.GetListenerCount());
	auto b = signal.Connect(std::bind(&VoidListenerObject::OnSignal, &listener_b));
	EXPECT_EQ(2, signal.GetListenerCount());

	signal.Emit();

	signal.Disconnect(a);
	EXPECT_EQ(1, signal.GetListenerCount());
	signal.Disconnect(b);
	EXPECT_EQ(0, signal.GetListenerCount());
}

//
void StaticVoidListener() { log("Void listener received signal"); }

TEST(Signal, VoidSignalToFunction) {
	Signal<void()> signal;

	auto id = signal.Connect(&StaticVoidListener);
	EXPECT_EQ(1, signal.GetListenerCount());

	signal.Emit();

	signal.Disconnect(id);
	EXPECT_EQ(0, signal.GetListenerCount());
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

TEST(Signal, SignalThroughInstanceVirtualMember) {
	Signal<void()> signal;

	std::unique_ptr<BaseListenerObject> listener(new InheritedListenerObject);

	auto id = signal.Connect(std::bind(&BaseListenerObject::OnSignal, listener.get()));
	EXPECT_EQ(1, signal.GetListenerCount());

	signal.Emit();
	EXPECT_TRUE(static_cast<InheritedListenerObject *>(listener.get())->received);

	signal.Disconnect(id);
	EXPECT_EQ(0, signal.GetListenerCount());
}

//
TEST(Signal, VoidSignalController) {
	Signal<void()> signal;

	bool signal_called = false;
	signal.Connect([&signal_called]() { signal_called = true; });

	bool controller_called = false;
	signal.Emit([&controller_called]() { controller_called = true; });

	EXPECT_TRUE(signal_called);
	EXPECT_TRUE(controller_called);
}

TEST(Signal, SignalController) {
	Signal<int()> signal;

	bool signal_called = false;
	signal.Connect([&signal_called]() -> int {
		signal_called = true;
		return 66;
	});

	int controller_result = 0;
	signal.Emit([&controller_result](int v) { controller_result = v; });

	EXPECT_TRUE(signal_called);
	EXPECT_EQ(66, controller_result);
}
