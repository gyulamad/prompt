#pragma once


#ifdef TEST
#include <string>
#include <vector>
#include <algorithm>









#endif

/*
TODO:
-----------------------------------
test_RingBufferEventQueue_*
They don’t cover:

Exception cases (e.g., invalid capacity), as the RingBuffer constructor already throws for capacity < 2, and we assume this is tested in the RingBuffer suite.
Multi-threaded scenarios (e.g., concurrent writes/reads), which could be added if needed but would require more complex setup.

---------------------------------------
test_EventBus_*
Not covered (potential additions):

Multi-threaded scenarios: Testing concurrent publish and delivery could be complex and might require a different framework.
Exception cases: No explicit exceptions are thrown by EventBus methods, so none are tested.
Destructor behavior: Testing thread cleanup in ~EventBus() would require mocking or observing thread state, which is tricky with cassert.

---------------------------------------------------
test_BaseEventConsumer_*
Not covered (potential additions):

Exception Cases: BaseEventConsumer doesn’t throw exceptions explicitly, so none are tested.
Multi-threaded Access: Could test concurrent event handling, but this requires a more complex setup beyond cassert.
Edge Cases: Like registering the same handler multiple times (currently, it just adds duplicates, which is fine but could be tested for specific behavior).

----------------------------------------
test_BaseEventProducer_*
Not covered (potential additions):

Exception Cases: BaseEventProducer doesn’t throw exceptions explicitly, so none are tested.
Concurrent Publishing: Multi-threaded publishing could be tested but requires a more complex setup.
Multiple Event Types: Tests only use TestEvent; adding another type could verify template flexibility.

----------------------------------------
test_BaseEventAgent_*
Not covered (potential additions):

Exception Cases: No explicit exceptions are thrown, so none are tested.
Concurrent Operations: Multi-threaded publish/consume scenarios could be added but require more setup.
Multiple Handlers: Could test registering multiple handlers for the same event type.

-----------------------------------
test_SelfMessageFilter_* and test_FilteredEventBus_*
Not covered (potential additions):

Async Delivery: Could test filtering with asyncDelivery = true, but it’s orthogonal to filtering logic.
Concurrent Filter Changes: Multi-threaded filter toggling could be tested but requires more setup.
Multiple Filters: Could test interactions between multiple custom filters.
*/