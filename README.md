# Issues

`thread_local` has static storage. One need to use a new type of
thread local with instance storage and hold whether a functor is
initialed per thred in the tasks. Otherwise, this would never work.





