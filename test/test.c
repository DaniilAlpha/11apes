#include "test_cpu.h"
#include "test_unibus.h"

int main() {
    test_unibus_run();
    test_cpu_run();
}
