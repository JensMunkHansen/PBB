Notable Issues with thread_local and C++20:
1. Destruction Order and Lifetime Issues
In C++20, there's a subtle mess around the lifetime of thread_local variables, especially:

If they depend on other thread_local variables (even indirectly).

If they interact with dynamically loaded/unloaded modules or shared libs.

This can lead to use-after-free in multithreaded applications when:

thread_local destructors run after shared resources they depended on are destroyed (like your custom queues, logs, pools).

This is notorious in environments like:

Linux dynamic libraries

Windows DLL unloading

ASan+TSan test harnesses

2. Order of Initialization
While C++20 didn't change this rule, it continues to bite people:

thread_local variables still have undefined order of initialization across translation units.

If your thread_local depends on a global (non-thread_local) variable, and they’re in separate TUs, kaboom 💥.

3. ABI & Sanitizer Bugs
GCC and Clang had (and some still have) ASan/TSan bugs in how thread_local is instrumented.

These often only manifest in long-running or multi-run test cases — exactly what you're doing.

Issues like GCC bug 92510 led to silent crashes or false UAFs.

4. Leak Sanitizer False Positives
If you're using -fsanitize=address,leak, you may get false positives due to thread_local values that are intentionally kept alive or not properly "leaked" to the leak sanitizer.

🧪 TL;DR:
If your issue shows up after hundreds of test runs, and you're using thread_local, then:

It could definitely be sanitizer tooling or shutdown-order-related.

Especially if you're seeing segfaults inside destructors or uninstrumented memory.

Adding small sleep()s before shutdown or avoiding destruction of the global pool sometimes "fixes" these, which is... telling.

If you're curious, we can dig into platform-specific fixes, or try a compile flag that delays destruction (__attribute__((used)), or wrapping in a Meyer's singleton that's intentionally never destroyed).

But until then — sleep tight 😴💻🐞
