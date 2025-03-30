#include <PBB/Memory.hpp>
#include <iostream>

struct Example
{
  int x;
  Example(int val)
  {
    if (val < 0)
      throw std::runtime_error("Negative value not allowed!");
    x = val;
  }
  ~Example() { std::cout << "Example destroyed\n"; }
};

int main()
{

  try
  {
    PBB::PaddedPlacement<Example> padded(-1); // Constructor throws!
  }
  catch (...)
  {
    std::cout << "Exception caught!\n";
  }
  return 0;
}
