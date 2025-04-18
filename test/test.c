#include "pdp11_cpu_test.h"
#include "unibus_test.h"

int main() {
    test_unibus_run();
    test_cpu_run();
}
