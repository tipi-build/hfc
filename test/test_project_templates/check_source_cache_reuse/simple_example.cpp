// Returns the sample library's variant id as the process exit code; the test
// reads it to detect which sources (pristine or locally modified cache) were
// actually compiled and linked. Rewritten by the test, kept for template
// validity.
#include <samplelib.h>

int main() {
  return samplelib_variant();
}
