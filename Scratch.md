You should use a .cpp file (out-of-line definition) if:

You want to guarantee a single vtable instantiation (important for DLL/shared-lib boundaries).

You rely on typeid or dynamic_cast across TU boundaries.

You're seeing linker errors for typeinfo, like undefined reference to typeinfo for IThreadTask.

If you’re getting typeinfo-related linker errors, putting the virtual method implementations (esp. destructor or first virtual function) in a .cpp file forces vtable emission and fixes it.



Why this matters for your std::weak_ptr<promise<T>>:
You're using a weak pointer to avoid calling .set_value() or .set_exception() on a destroyed std::promise, which is smart.

But, since rescheduling is possible at any point (even in the middle of stack unwinding), here are a few things to keep in mind:

1. Thread-local storage isn't migrated
If you're storing the promise or any state in thread-local variables, and the handler runs after rescheduling, it won’t see that data unless you reinitialize it.

2. Exception propagation is per-thread
Even if your thread hops between cores, the exception still travels up the same logical thread of execution—so as long as your state is on the stack or heap, it's safe.

3. Use of weak_ptr
If you're using weak_ptr to check if someone still cares about the result (e.g., a future waiting on the promise), that’s a solid pattern. You might do something like:

cpp
Copy
Edit
if (auto p = weakPromise.lock()) {
    p->set_exception(std::current_exception());
}
That way, if no one is waiting anymore, you don’t throw a broken_promise.



Is it possible that the std::promise is destroyed before the exception is caught and handled?

If the answer is no:
For example, if the promise is owned by the thread or the task itself, and it lives until after the task is complete (even if it throws)...

Then you don’t need a weak_ptr — you can safely call set_exception() because the promise is guaranteed to be alive.

If the answer is yes:
For example, if the promise is held by some external controller or a future consumer that may be destructed independently...

Then you do need a weak_ptr, or you risk a broken_promise when calling set_exception().

You still need weak_ptr if:
The promise might be destroyed before the task finishes (e.g., user cancels interest in the result by destroying the future).

You're in a situation where the future/promise lifecycle isn’t tightly coupled to task execution.

If the exceptions are collected to the true caller then I guess the future/promise is not tightly coupled to task execution


ChatGPT said:
Exactly — you're spot on. If you're collecting exceptions back at the "true caller" — meaning, somewhere outside the task itself — then the future/promise pair is detached from the task’s execution, and that's where weak_ptr becomes important.


std::shared_ptr<std::promise<int>> p = std::make_shared<std::promise<int>>();
std::future<int> f = p->get_future();

// user might destroy p before the task completes

std::weak_ptr<std::promise<int>> wp = p;

std::thread([wp] {
    try {
        throw std::runtime_error("boom");
    } catch (...) {
        if (auto sp = wp.lock()) {
            sp->set_exception(std::current_exception()); // safe only if still alive
        }
    }
}).detach();


Correct — p may not be alive when the background thread attempts to call set_exception(). That’s why using weak_ptr and checking with .lock() is important here.


If you want to guarantee that promise stays alive for the duration of the task, you’d need to pass a shared_ptr into the thread instead of a weak_ptr. But that means the background task holds a strong reference — and even if the user no longer wants the result, it will still be delivered.

It really depends on whether you want:

✅ Automatic cleanup if no one’s waiting (weak_ptr)

🔒 Guaranteed delivery of result or exception, even if no one cares anymore (shared_ptr)

Let me know which behavior you want to enforce — I can help you tweak the ownership model accordingly.


Fire-and-forget means:

The caller might not care about the result.

The future might be destroyed before the task finishes.

You don't want to hold onto resources (like the promise) longer than needed.

And you definitely don’t want exceptions like std::future_error from a broken_promise.


Summary:
You want weak ownership of the promise so that if the caller gives up, the task doesn’t keep it alive unnecessarily.

This lets you support:

fire-and-forget

optional results

safe exception handling without broken_promise

You’ve got the right instinct here. Let me know if you want a generic wrapper around this behavior — could be a nice utility to clean up repeated patterns.
