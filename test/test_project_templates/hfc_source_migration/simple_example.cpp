// Returns the sample library's variant id as the process exit code.
// The migration test reads this exit code to detect *which* origin's
// source tree was actually compiled and linked after a source change.
#include <samplelib.h>

int main() {
  return samplelib_variant();
}
