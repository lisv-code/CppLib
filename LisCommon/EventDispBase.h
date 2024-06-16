// ****** Events dispatcher base class. (c) 2024 LISV ******
#pragma once
#ifndef _LIS_EVENT_DISPATCH_BASE_H_
#define _LIS_EVENT_DISPATCH_BASE_H_

#include <functional>
#include <list>
#include <unordered_map>

// ** The class to be inherited.
// TDispatcher - type of the dispatcher (assuming the derived class).
// TEventType - type of an event (probably some enum).
// TEventData - type of data passed when the event raised (could be a pointer).
// Subscribe code example:
//   theDispatcher.EventSubscribe(event_type_1,
//     std::bind(&SubscriberClass::Event1_EventHandler,
//       &theSubscriber, std::placeholders::_1, std::placeholders::_2));
template <typename TDispatcher, typename TEventType, typename TEventData>
class EventDispatcherBase
{
public:
	struct EventInfo { TEventType type; TEventData data; };
	typedef int EventFunction(const TDispatcher* dispatcher, const EventInfo& evt_info);
	typedef std::function<EventFunction> EventHandler;
	typedef void* EventSubscriptionId; // TODO: consider using a shared pointer instead

	EventDispatcherBase() { }
	EventDispatcherBase(const EventDispatcherBase& src) noexcept : eventHandlers(src.eventHandlers) { }
	EventDispatcherBase(EventDispatcherBase&& src) noexcept : eventHandlers(std::move(src.eventHandlers)) { }
	virtual ~EventDispatcherBase() { }

	EventSubscriptionId EventSubscribe(TEventType type, EventHandler handler);
	bool EventUnsubscribe(EventSubscriptionId subscription_id);
	bool EventUnsubscribe(TEventType type);

protected:
	int RaiseEvent(TEventType type, TEventData data) const;

private:
	typedef std::list<EventHandler> HandlersContainer;
	std::unordered_map<TEventType, HandlersContainer> eventHandlers;

	static EventSubscriptionId GetSubscriptionId(const EventHandler& handler);

	static typename HandlersContainer::iterator
		FindHandler(HandlersContainer& list, EventSubscriptionId subscription_id);
};

// ******************************* EventDispatcherBase implementation ******************************

template<typename TDispatcher, typename TEventType, typename TEventData>
typename EventDispatcherBase<TDispatcher, TEventType, TEventData>::EventSubscriptionId
EventDispatcherBase<TDispatcher, TEventType, TEventData>::EventSubscribe(
	TEventType type, EventHandler handler)
{
	auto& list = eventHandlers[type];
	list.push_back(handler);
	return GetSubscriptionId(list.back());
}

template<typename TDispatcher, typename TEventType, typename TEventData>
bool EventDispatcherBase<TDispatcher, TEventType, TEventData>::EventUnsubscribe(
	EventSubscriptionId subscription_id)
{
	for (auto& evt : eventHandlers) {
		auto& list = evt.second;
		auto item = FindHandler(list, subscription_id);
		if (item != list.end()) {
			list.erase(item);
			return true;
		}
	}
	return false;
}

template<typename TDispatcher, typename TEventType, typename TEventData>
bool EventDispatcherBase<TDispatcher, TEventType, TEventData>::EventUnsubscribe(TEventType type)
{
	const auto item = eventHandlers.find(type);
	if (item != eventHandlers.end()) {
		(*item).second.clear();
		return true;
	}
	return false;
}

template<typename TDispatcher, typename TEventType, typename TEventData>
int EventDispatcherBase<TDispatcher, TEventType, TEventData>::RaiseEvent(
	TEventType type, TEventData data) const
{
	int result = 0;
	const auto& item = eventHandlers.find(type);
	if (item != eventHandlers.end()) {
		const auto& handlers = (*item).second;
		EventInfo evt_inf{};
		evt_inf.type = type;
		evt_inf.data = data;
		for (const auto& handler : handlers) {
			result = handler(static_cast<const TDispatcher*>(this), evt_inf);
			if (result < 0) return result;
		}
	}
	return result;
}

template<typename TDispatcher, typename TEventType, typename TEventData>
typename EventDispatcherBase<TDispatcher, TEventType, TEventData>::EventSubscriptionId
EventDispatcherBase<TDispatcher, TEventType, TEventData>::GetSubscriptionId(
	const EventHandler& handler)
{
	return (EventSubscriptionId)(&handler);
}

template<typename TDispatcher, typename TEventType, typename TEventData>
typename EventDispatcherBase<TDispatcher, TEventType, TEventData>::HandlersContainer::iterator
EventDispatcherBase<TDispatcher, TEventType, TEventData>::FindHandler(
	HandlersContainer& list, EventSubscriptionId subscription_id)
{
	for (auto it = list.begin(); it != list.end(); ++it) {
		if (GetSubscriptionId(*it) == subscription_id)
			return it;
	}
	return list.end();
}

#endif // #ifndef _LIS_EVENT_DISPATCH_BASE_H_
