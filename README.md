You should use a .cpp file (out-of-line definition) if:

You want to guarantee a single vtable instantiation (important for DLL/shared-lib boundaries).

You rely on typeid or dynamic_cast across TU boundaries.

You're seeing linker errors for typeinfo, like undefined reference to typeinfo for IThreadTask.

If you’re getting typeinfo-related linker errors, putting the virtual method implementations (esp. destructor or first virtual function) in a .cpp file forces vtable emission and fixes it.
