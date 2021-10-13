// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/assert.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace hg {

template <typename Signature>
class Signal;

template <typename T>
struct SignalControllerTraits {
	using Type = std::function<void(T)>;

	template <typename Func, typename... Args>
	static void Apply(const Type &controller, const Func &f, Args &&... args) {
		controller(f(args...));
	}
};

template <>
struct SignalControllerTraits<void> {
	using Type = std::function<void()>;

	template <typename Func, typename... Args>
	static void Apply(const Type &controller, const Func &f, Args &&... args) {
		f(args...);
		controller();
	}
};

template <typename Result, typename... Args>
class Signal<Result(Args...)> {
	using Listener = std::function<Result(Args...)>;

public:
	struct Connection {
		Connection() = default;
		explicit Connection(Listener *l_) : l(l_) {}
		explicit operator bool() const { return l != nullptr; }
		Listener *l{nullptr};
	};

	Connection Connect(Listener l) {
		std::lock_guard<std::mutex> guard(lock);
		auto c = std::make_shared<Listener>(std::move(l));
		listeners.push_back(c);
		return Connection(c.get());
	}
	
	void Disconnect(Connection cnx) {
		std::lock_guard<std::mutex> guard(lock);
		__ASSERT__(std::count_if(listeners.begin(), listeners.end(), [&cnx](const std::shared_ptr<Listener> &c) { return c.get() == cnx.l; }) > 0);
		listeners.erase(std::remove_if(listeners.begin(), listeners.end(), [&cnx](const std::shared_ptr<Listener> &c) { return c.get() == cnx.l; }), listeners.end());
	}

	void DisconnectAll() {
		std::lock_guard<std::mutex> guard(lock);
		listeners.clear();
	}

	void Emit(Args... args) const {
		lock.lock();
		auto local_listeners = listeners; // allow changes while emitting
		lock.unlock();

		for (auto &listener : local_listeners)
			(*listener)(args...);
	}

	void Emit(const typename SignalControllerTraits<Result>::Type &controller, Args... args) const {
		lock.lock();
		auto local_listeners = listeners; // allow changes while emitting
		lock.unlock();

		for (auto &listener : local_listeners)
			SignalControllerTraits<Result>::Apply(controller, *listener, args...);
	}

	size_t GetListenerCount() const {
		std::lock_guard<std::mutex> guard(lock);
		return listeners.size();
	}

	std::vector<std::shared_ptr<Listener>> GetListeners() const {
		std::lock_guard<std::mutex> guard(lock);
		return listeners;
	}

private:
	mutable std::mutex lock;
	std::vector<std::shared_ptr<Listener>> listeners;
};

} // namespace hg
